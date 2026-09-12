"""Cross-link the Windows embed archive with real x64 and ARM64 COFF objects."""

import importlib.util
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "build_windows", ROOT / "tools/stanc_embed/build_windows.py")
BUILDER = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(BUILDER)
TOOLS = ("clang", "llvm-ar", "llvm-nm", "lld-link", "llvm-readobj")


@unittest.skipUnless(all(shutil.which(tool) for tool in TOOLS), "requires LLVM tools")
class WindowsEmbedTest(unittest.TestCase):
    def test_archive_links_with_lld_on_both_architectures(self):
        for architecture in ("x86_64", "aarch64"):
            with self.subTest(architecture=architecture), tempfile.TemporaryDirectory() as tmp:
                root = Path(tmp) / "path with spaces"
                root.mkdir()
                compiler = ["clang", f"--target={architecture}-w64-windows-gnu"]

                def compile_object(directory, source):
                    directory.mkdir(exist_ok=True)
                    src = directory / "member.c"
                    obj = src.with_suffix(".o")
                    src.write_text(source)
                    subprocess.run([*compiler, "-c", src, "-o", obj], check=True)
                    return obj

                def archive(name, *objects):
                    path = root / name
                    subprocess.run(["llvm-ar", "rcs", path, *objects], check=True)
                    return path

                native = compile_object(root / "native", """
                    int runtime_data = 7;
                    int runtime_function(void) { return runtime_data; }
                    extern int caml_program(void);
                    int caml_startup(void) { return caml_program(); }
                """)
                caller = compile_object(root / "caller", """
                    __declspec(dllimport) int runtime_function(void);
                    __declspec(dllimport) int runtime_data;
                    __declspec(dllimport) int crt_function(void);
                    __declspec(dllimport) int external_os(void);
                    extern void *static_symtable;
                    int caml_program(void) {
                        return runtime_function() + runtime_data + crt_function() + external_os()
                               + (static_symtable != 0);
                    }
                """)
                unused = compile_object(root / "unused", """
                    __declspec(dllimport) int unused_native(void);
                    int unused_stub(void) { return unused_native(); }
                """)
                unused_native = compile_object(root / "unused_native", """
                    extern int unavailable_dependency(void);
                    int unused_native(void) { return unavailable_dependency(); }
                """)
                crt = compile_object(root / "crt", "int crt_function(void) { return 3; }")
                crt_archive = archive("crt.a", crt)
                external = compile_object(root / "external", "int external_os(void) { return 1; }")
                external_imports = root / "external.lib"
                subprocess.run(["lld-link", "/dll", "/noentry", "/nodefaultlib",
                                "/export:external_os", f"/out:{root / 'external.dll'}",
                                f"/implib:{external_imports}", external], check=True)
                stubs = [archive("stubs.a", caller, unused),
                         archive("other.a", unused_native)]
                embed = root / "stanc_embed.a"
                # Rebuilding the same destination must not accumulate members.
                for _ in range(2):
                    BUILDER.build_static_archive(embed, [native], stubs, compiler, crt_archive)
                members = subprocess.check_output(["llvm-ar", "t", embed], text=True).splitlines()
                # All inputs deliberately share a filename; flattening must keep them all.
                self.assertEqual(members.count("member.o"), 4)
                symbols = subprocess.check_output(["llvm-nm", "--defined-only", embed], text=True)
                for symbol in ("__imp_runtime_function", "__imp_runtime_data", "__imp_crt_function"):
                    self.assertIn(symbol, symbols)
                self.assertNotIn("__imp_external_os", symbols)

                entry = compile_object(root / "entry", """
                    extern int caml_startup(void);
                    void entry(void) { (void)caml_startup(); }
                """)
                executable = root / "embed.exe"
                subprocess.run(["lld-link", "/entry:entry", "/subsystem:console",
                                "/nodefaultlib", "/opt:noref", f"/out:{executable}",
                                entry, embed, crt_archive, external_imports], check=True)
                headers = subprocess.check_output(
                    ["llvm-readobj", "--file-headers", executable], text=True)
                self.assertIn("IMAGE_FILE_MACHINE_AMD64" if architecture == "x86_64"
                              else "IMAGE_FILE_MACHINE_ARM64", headers)


if __name__ == "__main__":
    unittest.main()
