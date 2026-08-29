#!/usr/bin/env python3
"""Generate the synthetic ODE sensitivity-ceiling matrix.

The generated models deliberately use the public modern ODE interfaces. State
count, active parameter width, and output-time count remain data dimensions so
one optimized MIR file per (solver, branch, initial-state activity, parameter
activity) tuple covers the whole shape sweep; stanli specializes those
dimensions while compiling against each JSON file. The runnable manifest has
84 rows spanning the three activity masks that survive in the log-density
graph. Four genuine data/data rows live in compile_only.json: their ODEs are
correctly precomputed out of that graph, documenting the fourth mask without
mislabeling a parameter-dependent input as data.

This is developer measurement infrastructure, not a corpus or production
eligibility policy. It writes a manifest consumed by bench_ode_ceiling.
"""

import argparse
import hashlib
import json
import pathlib
import re
import subprocess


SOLVERS = ("rk45", "ckrk", "bdf", "adams")
SENTINEL = ".stanli-ode-ceiling-generator"
SENTINEL_TEXT = "stanli synthetic ODE ceiling generator v1\n"

SOURCE_NAME = re.compile(
    r"synthetic_(?:rk45|ckrk|bdf|adams)_"
    r"(?:branch|straight)_(?:yactive|ydata)_(?:thetaactive|thetadata)"
    r"\.(?:stan|hpp|tmir\.sexp)"
)
DATA_NAME = re.compile(
    r"synthetic_(?:rk45|ckrk|bdf|adams)_"
    r"(?:branch|straight)_(?:yactive|ydata)_(?:thetaactive|thetadata)_"
    r"s\d+_p\d+_n\d+\.json"
)


def sha256_file(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def model_source(
    solver: str, branched: bool, active_y0: bool, active_theta: bool
) -> str:
    branch_body = """
      if (t > 1.0) {
        dy[i] = base + 0.001 * y[i];
      } else {
        dy[i] = base - 0.0015 * y[i];
      }
""" if branched else "      dy[i] = base;\n"
    y0_data = "" if active_y0 else "  vector[S] y0_data;\n"
    y0_param = "  vector[S] y0;\n" if active_y0 else ""
    y0_name = "y0" if active_y0 else "y0_data"
    y0_prior = "  y0 ~ normal(0.2, 0.5);\n" if active_y0 else ""
    theta_data = "" if active_theta else "  vector[P] theta_data;\n"
    theta_param = "  vector[P] theta;\n" if active_theta else ""
    theta_name = "theta" if active_theta else "theta_data"
    theta_prior = "  theta ~ normal(0, 0.5);\n" if active_theta else ""
    return f"""functions {{
  vector synthetic_rhs(real t, vector y, vector theta, real theta_scale) {{
    vector[rows(y)] dy;
    real forcing = sum(theta) * theta_scale;
    real coupling = 0.02 * sum(y) / rows(y);
    for (i in 1:rows(y)) {{
      real base = -0.12 * y[i] + coupling + 0.01 * forcing;
{branch_body}    }}
    return dy;
  }}
}}
data {{
  int<lower=1> S;
  int<lower=0> P;
  int<lower=1> N;
  array[N] real<lower=0> ts;
  real<lower=0> rel_tol;
  real<lower=0> abs_tol;
  real<lower=0> theta_scale;
{y0_data}{theta_data}}}
parameters {{
{y0_param}{theta_param}
  real nuisance;
}}
transformed parameters {{
  array[N] vector[S] z = ode_{solver}_tol(
      synthetic_rhs, {y0_name}, 0.0, ts, rel_tol, abs_tol, 1000000,
      {theta_name}, theta_scale);
}}
model {{
{y0_prior}  nuisance ~ normal(0, 1);
{theta_prior}
  for (n in 1:N) target += 0.01 * sum(z[n]);
}}
"""


def cases():
    """Twenty-one runnable cases per solver across masks 0x3, 0x2, 0x1."""
    baseline = (2, 4, 8, False, True, True)
    rows = {baseline: "baseline"}
    for states in (1, 2, 4, 8, 16):
        rows.setdefault((states, 4, 8, False, True, True), "states")
    for params in (0, 1, 4, 16, 64):
        rows.setdefault((2, params, 8, False, True, True), "parameters")
    for times in (1, 4, 8, 32, 128):
        rows.setdefault((2, 4, times, False, True, True), "output_times")
    rows.setdefault((2, 4, 8, True, True, True), "branch")
    rows.setdefault((2, 4, 8, False, False, True), "y0_activity")
    rows.setdefault((2, 4, 8, False, True, False), "theta_activity")

    interactions = (
        (1, 64, 1, False, False, True),
        (16, 0, 128, False, True, True),
        (8, 32, 32, True, True, True),
        (2, 64, 32, True, False, True),
        (2, 0, 8, False, False, True),
    )
    for row in interactions:
        rows[row] = "interaction"
    return [(s, p, n, branch, active_y0, active_theta, axis)
            for (s, p, n, branch, active_y0, active_theta), axis
            in rows.items()]


def known_generated_file(path: pathlib.Path) -> bool:
    """Whether path names a regular file this generator may have created."""
    if path.is_symlink() or not path.is_file():
        return False
    return (path.name in ("manifest.json", "compile_only.json")
            or SOURCE_NAME.fullmatch(path.name) is not None
            or DATA_NAME.fullmatch(path.name) is not None)


def prepare_output(output: pathlib.Path, force: bool) -> None:
    """Create output or safely clear a directory owned by this generator."""
    if output.is_symlink():
        raise ValueError(f"refusing symlink output directory: {output}")
    if output.exists() and not output.is_dir():
        raise NotADirectoryError(output)

    if not output.exists():
        output.mkdir(parents=True)
    else:
        entries = list(output.iterdir())
        if entries:
            marker = output / SENTINEL
            marker_valid = (
                not marker.is_symlink()
                and marker.is_file()
                and marker.read_text(encoding="utf-8") == SENTINEL_TEXT
            )
            if not marker_valid:
                raise FileExistsError(
                    f"refusing nonempty directory not owned by this "
                    f"generator: {output}"
                )
            if not force:
                raise FileExistsError(
                    f"refusing to replace generated directory {output}; "
                    "pass --force"
                )
            unknown = [
                child for child in entries
                if child.name != SENTINEL and not known_generated_file(child)
            ]
            if unknown:
                names = ", ".join(sorted(child.name for child in unknown))
                raise FileExistsError(
                    f"refusing to clean unknown or non-regular entries in "
                    f"{output}: {names}"
                )
            for child in entries:
                if child.name != SENTINEL:
                    child.unlink()

    marker = output / SENTINEL
    if marker.is_symlink():
        raise ValueError(f"refusing symlink sentinel: {marker}")
    marker.write_text(SENTINEL_TEXT, encoding="utf-8")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("output", type=pathlib.Path)
    parser.add_argument("--stanc", type=pathlib.Path,
                        default=pathlib.Path("deps/stanc3/stanc"))
    parser.add_argument(
        "--force", action="store_true",
        help="replace only a sentinel-marked directory of generated files",
    )
    args = parser.parse_args()

    output = args.output.expanduser().absolute()
    stanc = args.stanc.expanduser().resolve()
    if not stanc.is_file():
        raise FileNotFoundError(stanc)
    stanc_sha256 = sha256_file(stanc)
    prepare_output(output, args.force)
    manifest = []
    compile_only = []

    sources = {}
    for solver in SOLVERS:
        for (states, params, times, branched, active_y0, active_theta,
             axis) in cases():
            source_key = (solver, branched, active_y0, active_theta)
            y_activity = "yactive" if active_y0 else "ydata"
            theta_activity = "thetaactive" if active_theta else "thetadata"
            stem = (f"synthetic_{solver}_"
                    f"{'branch' if branched else 'straight'}_{y_activity}_"
                    f"{theta_activity}")
            stan = output / f"{stem}.stan"
            mir = output / f"{stem}.tmir.sexp"
            if source_key not in sources:
                stan.write_text(
                    model_source(solver, branched, active_y0, active_theta),
                    encoding="utf-8",
                )
                generated = subprocess.run(
                    [str(stanc), "--O1", "--debug-optimized-mir",
                     str(stan)], check=True, capture_output=True, text=True)
                mir.write_text(generated.stdout, encoding="utf-8")
                sources[source_key] = (stan, mir)
            else:
                stan, mir = sources[source_key]

            case = f"{stem}_s{states}_p{params}_n{times}"
            data = output / f"{case}.json"
            values = {
                "S": states,
                "P": params,
                "N": times,
                "ts": [2.0 * (i + 1) / times for i in range(times)],
                "rel_tol": 1e-8,
                "abs_tol": 1e-8,
                "theta_scale": 1.0 / (params + 1.0),
            }
            if not active_y0:
                values["y0_data"] = [0.2 + 0.01 * i for i in range(states)]
            if not active_theta:
                values["theta_data"] = [
                    0.05 * (i + 1) / (params + 1.0)
                    for i in range(params)
                ]
            data.write_text(
                json.dumps(values, separators=(",", ":")), encoding="utf-8"
            )
            manifest.append({
                "case": case,
                "solver": solver,
                "branched": branched,
                "active_y0": active_y0,
                "active_theta": active_theta,
                "states": states,
                "parameters": params,
                "output_times": times,
                "axis": axis,
                "require_exact": True,
                "mir": str(mir),
                "data": str(data),
                "stan": str(stan),
                "stan_sha256": sha256_file(stan),
                "stanc": str(stanc),
                "stanc_sha256": stanc_sha256,
            })

    # With both ODE scalar inputs data, Stanli evaluates the solve in its
    # write/preparation path and removes it from the repeated log-density
    # graph. Preserve one real source/MIR/data row per solver as an explicit
    # compile-only disposition instead of feeding a no-OP model to the timing
    # binaries or manufacturing a false activity bit with a control input.
    for solver in SOLVERS:
        stem = f"synthetic_{solver}_straight_ydata_thetadata"
        stan = output / f"{stem}.stan"
        mir = output / f"{stem}.tmir.sexp"
        stan.write_text(
            model_source(solver, False, False, False), encoding="utf-8"
        )
        generated = subprocess.run(
            [str(stanc), "--O1", "--debug-optimized-mir", str(stan)],
            check=True,
            capture_output=True,
            text=True,
        )
        mir.write_text(generated.stdout, encoding="utf-8")
        case = f"{stem}_s2_p4_n8"
        data = output / f"{case}.json"
        data.write_text(
            json.dumps({
                "S": 2,
                "P": 4,
                "N": 8,
                "ts": [0.25 * (i + 1) for i in range(8)],
                "rel_tol": 1e-8,
                "abs_tol": 1e-8,
                "theta_scale": 0.2,
                "y0_data": [0.2, 0.21],
                "theta_data": [0.01, 0.02, 0.03, 0.04],
            }, separators=(",", ":")),
            encoding="utf-8",
        )
        compile_only.append({
            "case": case,
            "solver": solver,
            "branched": False,
            "active_y0": False,
            "active_theta": False,
            "type_mask": "0x0",
            "states": 2,
            "parameters": 4,
            "output_times": 8,
            "log_density_graph_op_ode": False,
            "disposition": (
                "constant-folded by Stanli outside the repeated "
                "log-density graph"
            ),
            "mir": str(mir),
            "data": str(data),
            "stan": str(stan),
            "stan_sha256": sha256_file(stan),
            "stanc": str(stanc),
            "stanc_sha256": stanc_sha256,
        })

    manifest_path = output / "manifest.json"
    manifest_path.write_text(
        json.dumps(manifest, indent=2) + "\n", encoding="utf-8"
    )
    compile_only_path = output / "compile_only.json"
    compile_only_path.write_text(
        json.dumps(compile_only, indent=2) + "\n", encoding="utf-8"
    )
    print(
        f"generated {len(manifest)} runnable cases and "
        f"{len(compile_only)} compile-only cases in {output}"
    )
    print(manifest_path)
    print(compile_only_path)


if __name__ == "__main__":
    main()
