"""Bundle Windows OCaml objects without GNU partial linking (unsupported by LLD)."""

import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys
import tempfile


def output(*args, **kwargs):
    return subprocess.check_output(args, text=True, **kwargs).strip()


def build_static_archive(destination, objects, archives, compiler, c_runtime):
    """Keep members lazy so unused OCaml stubs do not add link dependencies."""
    with tempfile.TemporaryDirectory(prefix="stanc-archive-") as tmp:
        tmp = Path(tmp)
        combined = tmp / "embed.a"
        # L flattens input archives, including members with duplicate filenames.
        subprocess.run(["llvm-ar", "qcLs", combined, *objects, *archives], check=True)

        def symbols(option, *paths):
            return {line.split()[0] for line in output(
                "llvm-nm", "--extern-only", "--no-sort", "--format=posix", option,
                *(str(path) for path in paths)).splitlines() if line.split()}

        defined = symbols("--defined-only", combined, c_runtime)
        undefined = symbols("--undefined-only", combined)
        imports = sorted(name for name in undefined
                         if name.startswith("__imp_") and name[6:] in defined
                         and name not in defined)
        # C stubs can reference native OCaml definitions through DLL-import
        # pointers. Each pointer needs its own member: extracting one must not
        # pull in every otherwise-unused stub and its external dependencies.
        definitions = [("static_symtable", "0")]
        definitions.extend((name, name[6:]) for name in imports)
        import_objects = []
        for index, (name, target) in enumerate(definitions):
            assembly = tmp / f"import_{index}.s"
            obj = assembly.with_suffix(".o")
            assembly.write_text('.section .rdata,"dr"\n.balign 8\n'
                                f'.globl {name}\n{name}:\n.quad {target}\n')
            subprocess.run([*compiler, "-c", assembly, "-o", obj], check=True)
            import_objects.append(obj.name)
        subprocess.run(["llvm-ar", "qs", combined, *import_objects], cwd=tmp, check=True)
        shutil.copyfile(combined, destination)


def main():
    source = Path(sys.argv[1]).resolve()
    target = "src/stanc_embed/stanc_embed.exe.o"
    rules = json.loads(output("dune", "describe", "rules", "--root", str(source),
                              "--format=json", "--profile=release", target))
    rule, = rules
    dependencies = [dep["File"] for dep in rule["deps"] if "File" in dep]
    subprocess.run(["dune", "build", "--root", source, "--profile=release",
                    "-j", sys.argv[2],
                    *(path for kind, path in dependencies if kind == "In_build_dir")],
                   check=True)

    _, directory, action = rule["action"]
    command = action[1:]
    workdir = source / directory
    ocamlopt = command[0]
    stdlib = Path(output(ocamlopt, "-where"))
    libraries = [workdir / arg for arg in command if arg.endswith(".cmxa")]
    library_dirs = [lib.parent for lib in libraries] + [stdlib]
    archives = []
    for library in libraries:
        info = output(str(Path(ocamlopt).with_name("ocamlobjinfo.exe")), str(library))
        c_objects = next(line.removeprefix("Extra C object files:")
                         for line in info.splitlines() if line.startswith("Extra C object files:"))
        for arg in shlex.split(c_objects):
            name = "lib" + arg[2:] + ".a" if arg.startswith("-l") else arg
            archive = next((path / name for path in library_dirs if (path / name).is_file()), None)
            # System import libraries stay unresolved until CMake's final link.
            if archive is not None and archive not in archives:
                archives.append(archive)
    archives.append(stdlib / "libasmrun.a")
    # The stock runtime references this support object; no flexlink invocation
    # is needed when its symbols and the empty static symbol table are linked in.
    runtime_support = stdlib / "flexdll" / (
        "flexdll_" + output(ocamlopt, "-config-var", "system") + ".o")
    compiler = os.environ.get("CC", "clang")
    c_runtime = Path(output(compiler, "-print-file-name=libmsvcrt.a"))

    destination = source / "_build/stanc_embed.static.a"
    with tempfile.TemporaryDirectory(prefix="stanc-static-") as tmp:
        tmp = Path(tmp)
        command = ["-output-obj" if arg == "-output-complete-obj" else arg
                   for arg in command]
        command[command.index("-o") + 1] = str(tmp / "ocaml.o")
        subprocess.run(command, cwd=workdir, check=True)
        build_static_archive(destination, [tmp / "ocaml.o", runtime_support],
                             archives, [compiler], c_runtime)


if __name__ == "__main__":
    main()
