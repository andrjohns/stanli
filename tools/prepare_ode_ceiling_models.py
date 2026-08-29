#!/usr/bin/env python3
"""Prepare the three real ODE ceiling inputs and a benchmark manifest.

The script extracts pinned PosteriorDB data, asks the pinned stanc executable
for optimized MIR, and emits rows for RK45/CKRK and BDF/Adams.  It never
overwrites an unowned directory.
"""

import argparse
import hashlib
import json
import pathlib
import subprocess
import zipfile


REPO = pathlib.Path(__file__).resolve().parent.parent
SENTINEL = ".stanli-ode-ceiling-models-owned"
SENTINEL_TEXT = "owned by prepare_ode_ceiling_models.py v1\n"
MODELS = (
    ("lotka_volterra", "hudson_lynx_hare", ("rk45", "ckrk")),
    ("soil_incubation", "soil_carbon", ("rk45", "ckrk")),
    ("one_comp_mm_elim_abs", "one_comp_mm_elim_abs", ("bdf", "adams")),
)


def sha256_file(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def prepare_output(path: pathlib.Path, force: bool) -> None:
    if path.is_symlink():
        raise RuntimeError(f"refusing symlink output directory: {path}")
    if path.exists() and not path.is_dir():
        raise RuntimeError(f"output exists and is not a directory: {path}")
    if not path.exists():
        path.mkdir(parents=True)
        (path / SENTINEL).write_text(SENTINEL_TEXT)
        return

    sentinel = path / SENTINEL
    children = list(path.iterdir())
    generated = [child for child in children if child.name != SENTINEL]
    if not generated:
        sentinel_present = sentinel.exists() or sentinel.is_symlink()
        if sentinel_present:
            if sentinel.is_symlink() or not sentinel.is_file():
                raise RuntimeError(f"invalid ownership sentinel: {sentinel}")
            if sentinel.read_text() != SENTINEL_TEXT:
                raise RuntimeError(
                    f"invalid ownership sentinel contents: {sentinel}"
                )
        else:
            sentinel.write_text(SENTINEL_TEXT)
        return
    if not force:
        raise FileExistsError(f"refusing nonempty output directory: {path}")
    if sentinel.is_symlink() or not sentinel.is_file():
        raise RuntimeError(f"refusing to clean unowned output directory: {path}")
    if sentinel.read_text() != SENTINEL_TEXT:
        raise RuntimeError(f"refusing invalid ownership sentinel: {sentinel}")
    for child in generated:
        allowed = (
            child.name == "manifest.json"
            or child.name.endswith(".tmir.sexp")
            or child.name.endswith(".data.json")
        )
        if child.is_symlink() or not child.is_file() or not allowed:
            raise RuntimeError(f"refusing unexpected output entry: {child}")
    for child in generated:
        child.unlink()


def extract_json(archive: pathlib.Path) -> bytes:
    with zipfile.ZipFile(archive) as source:
        members = [
            name for name in source.namelist()
            if name.endswith(".json") and not name.endswith("/")
        ]
        if len(members) != 1:
            raise RuntimeError(
                f"expected one JSON member in {archive}, found {len(members)}"
            )
        return source.read(members[0])


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=pathlib.Path)
    parser.add_argument(
        "--stanc", type=pathlib.Path, default=REPO / "deps/stanc3/stanc"
    )
    parser.add_argument(
        "--posteriordb",
        type=pathlib.Path,
        default=REPO / "deps/posteriordb/posterior_database",
    )
    parser.add_argument("--force", action="store_true")
    args = parser.parse_args()

    output = args.output.expanduser().absolute()
    stanc = args.stanc.expanduser().resolve()
    posteriordb = args.posteriordb.expanduser().resolve()
    if not stanc.is_file():
        raise FileNotFoundError(stanc)
    prepare_output(output, args.force)
    stanc_sha256 = sha256_file(stanc)

    manifest = []
    for model, data_name, solvers in MODELS:
        stan = posteriordb / "models/stan" / f"{model}.stan"
        archive = posteriordb / "data/data" / f"{data_name}.json.zip"
        if not stan.is_file():
            raise FileNotFoundError(stan)
        if not archive.is_file():
            raise FileNotFoundError(archive)
        stan_sha256 = sha256_file(stan)
        archive_sha256 = sha256_file(archive)

        data = output / f"{model}.data.json"
        mir = output / f"{model}.tmir.sexp"
        data.write_bytes(extract_json(archive))
        try:
            stan_arg = stan.relative_to(REPO)
        except ValueError:
            stan_arg = stan
        generated = subprocess.run(
            [str(stanc), "--O1", "--debug-optimized-mir", str(stan_arg)],
            check=True,
            capture_output=True,
            text=True,
            cwd=REPO,
        )
        mir.write_text(generated.stdout)
        for solver in solvers:
            manifest.append(
                {
                    "case": f"{model}_{solver}",
                    "model": model,
                    "solver": solver,
                    "mir": str(mir),
                    "data": str(data),
                    "stan": str(stan.resolve()),
                    "stan_sha256": stan_sha256,
                    "source_archive": str(archive.resolve()),
                    "source_archive_sha256": archive_sha256,
                    "stanc": str(stanc),
                    "stanc_sha256": stanc_sha256,
                }
            )

    manifest_path = output / "manifest.json"
    manifest_path.write_text(json.dumps(manifest, indent=2) + "\n")
    print(f"prepared {len(MODELS)} models and {len(manifest)} solver rows")
    print(manifest_path)


if __name__ == "__main__":
    main()
