#!/usr/bin/env python3
"""Tests for the Python package: the ctypes layer and its error paths.

Run against an installed wheel (CI does) or a local build:

    tools/build_wheel.sh && PYTHONPATH=python python3 tests/test_python.py
"""
import base64
import concurrent.futures
import contextlib
import io
import pathlib
import re
import shutil
import subprocess
import sys
import tempfile
import threading
from unittest import mock

import numpy as np

import stanli

FIXTURES = pathlib.Path(__file__).resolve().parent / "fixtures"


def _portable_payload(mir):
    assert mir.startswith("STANLI2:"), mir[:80]
    return base64.b64decode(mir[8:], validate=True)


def test_sample_eight_schools():
    m = stanli.Model(stan_file=FIXTURES / "es.stan",
                     data=FIXTURES / "eight_schools.json")
    d = m.sample(seed=1, warmup=1000, samples=1000, refresh=0)
    mu = d["mu"].mean()
    assert 3.0 < mu < 6.0, mu


def test_log_prob_grad():
    m = stanli.Model(stan_file=FIXTURES / "es.stan",
                     data=FIXTURES / "eight_schools.json")
    lp, grad = m.log_prob_grad(np.zeros(m.n_unconstrained))
    assert np.isfinite(lp), lp
    assert grad.shape == (m.n_unconstrained,)
    assert np.isfinite(grad).all(), grad


def test_unconstrain_round_trip():
    # The forward direction is the oracle: constrain a free point into the
    # CSV row, hand the constrained parameters back by name, and require the
    # free point out. initrt carries an array of simplexes and an array of
    # matrices, whose serial order differs from the free vector's.
    m = stanli.Model(stan_file=FIXTURES / "initrt.stan",
                     data=FIXTURES / "initrt.json")
    q = np.ascontiguousarray(
        [0.1 + 0.05 * (i % 7) - 0.15 * (i % 3)
         for i in range(m.n_unconstrained)], dtype=np.float64)

    # The forward direction has no public wrapper yet, so drive the C entry
    # point the module already binds. The point of this test is unconstrain().
    names, _ = m._column_names()
    row = np.empty(len(names))
    rc = stanli._lib.stanli_wa_row(m._m, stanli._dptr(q), stanli._dptr(row))
    assert rc == 0

    values = {}
    for name, value in zip(names, row):
        values.setdefault(name.split(".")[0], []).append(float(value))
    values = {k: (v[0] if len(v) == 1 else v) for k, v in values.items()}

    back = m.unconstrain(values)
    assert np.allclose(back, q, atol=1e-9), (back, q)


def test_unconstrain_names_a_missing_parameter():
    m = stanli.Model(stan_file=FIXTURES / "initrt.stan",
                     data=FIXTURES / "initrt.json")
    try:
        m.unconstrain({"mu": 0.0})
    except ValueError as e:
        assert "sigma" in str(e), str(e)
        return
    raise AssertionError("expected ValueError for an incomplete document")


def test_failing_gradient_raises():
    # normal with sigma = -1 passes compilation but throws inside stan-math
    # at evaluation time; the wrapper must raise, not hand back an
    # uninitialized gradient buffer.
    m = stanli.Model(
        stan_code="parameters { real x; } model { x ~ normal(0, -1); }")
    try:
        m.log_prob_grad(np.zeros(1))
    except RuntimeError:
        return
    raise AssertionError("expected RuntimeError from a failing gradient")


def test_sample_returns_transformed_parameters():
    # es.stan declares `vector[J] theta` in transformed parameters; the
    # write_array path must surface it alongside the declared parameters.
    m = stanli.Model(stan_file=FIXTURES / "es.stan",
                     data=FIXTURES / "eight_schools.json")
    d = m.sample(seed=1, warmup=200, samples=100, refresh=0)
    for col in ("mu", "tau", "theta.1", "theta.8"):
        assert col in d, sorted(d)[:12]
    assert abs(d["theta.1"].mean()) < 30.0


def test_sample_returns_generated_quantities():
    # This model's per-draw branch selects the interpreted write_array; its
    # columns and seeded RNG stream must both reach Python.
    code = (FIXTURES / "gqrng.stan").read_text()
    m = stanli.Model(stan_code=code, data={"N": 5})
    d = m.sample(seed=7, warmup=200, samples=50, refresh=0)
    for col in ("sigma", "yrep", "crep", "branchy", "p"):
        assert col in d, sorted(d)
    crep = d["crep"]
    assert ((crep == np.floor(crep)) & (crep >= 0) & (crep <= 5)).all()
    assert (d["p"] == 6.0).all()
    assert d["yrep"].std() > 0.0
    progress = io.StringIO()
    with contextlib.redirect_stdout(progress):
        d2 = stanli.Model(stan_code=code, data={"N": 5}).sample(
            seed=7, warmup=200, samples=50, refresh=1)
    assert "Iteration:" in progress.getvalue()
    assert np.array_equal(d.draws(), d2.draws()), \
        "progress changed parameter or generated-quantity draws"
    assert np.array_equal(d.sampler_stats, d2.sampler_stats), \
        "progress changed sampler statistics"


def _es():
    return stanli.Model(stan_file=FIXTURES / "es.stan",
                        data=FIXTURES / "eight_schools.json")


def _normal():
    return stanli.Model(
        stan_code="parameters { real x; } model { x ~ normal(0, 1); }")


def test_sample_progress_output_and_refresh_zero():
    # The formatter is Python-side so redirect_stdout works in notebooks and
    # test runners rather than only at the process file-descriptor level.
    exact = io.StringIO()
    with contextlib.redirect_stdout(exact):
        stanli._print_sample_progress(1, 1, 2000, True)
    assert exact.getvalue() == (
        "Chain [1] Iteration:    1 / 2000 [  0%]  (Warmup)\n")

    shown = io.StringIO()
    with contextlib.redirect_stdout(shown):
        fit = _normal().sample(chains=1, warmup=4, samples=3,
                               parallel_chains=1, refresh=2)
    assert fit.n_draws == 3
    text = shown.getvalue()
    lines = [line for line in text.splitlines() if "Iteration:" in line]
    iterations = [int(re.search(r"Iteration:\s+(\d+)", line).group(1))
                  for line in lines]
    assert iterations == [1, 2, 4, 5, 6, 7], lines
    assert all("(Warmup)" in line for line in lines[:3])
    assert all("(Sampling)" in line for line in lines[3:])
    assert "Elapsed Time:" in text
    assert "seconds (Warm-up)" in text
    assert "seconds (Sampling)" in text
    assert "seconds (Total)" in text

    quiet = io.StringIO()
    with contextlib.redirect_stdout(quiet):
        _normal().sample(chains=1, warmup=4, samples=3,
                         parallel_chains=1, refresh=0)
    assert quiet.getvalue() == ""


def test_sample_progress_callback_runs_on_the_calling_thread():
    owner = threading.get_ident()

    class SameThreadStream(io.StringIO):
        def write(self, text):
            assert threading.get_ident() == owner, \
                "Python progress callback ran on a sampler worker"
            return super().write(text)

    stream = SameThreadStream()
    with contextlib.redirect_stdout(stream):
        _normal().sample(chains=2, warmup=20, samples=10,
                         parallel_chains=2, refresh=7)

    seen = {1: [], 2: []}
    for line in stream.getvalue().splitlines():
        match = re.search(r"Chain \[(\d+)\] Iteration:\s+(\d+)", line)
        if match:
            seen[int(match.group(1))].append(int(match.group(2)))
    assert all(seen.values()), seen
    assert all(values == sorted(values) for values in seen.values()), seen


def test_sample_refresh_validation():
    m = _normal()
    for bad, error in [(-1, ValueError), (1.5, TypeError), ("2", TypeError),
                       (True, TypeError), (np.bool_(False), TypeError),
                       (None, TypeError), (2 ** 31, OverflowError)]:
        try:
            m.sample(chains=1, warmup=0, samples=1, refresh=bad)
        except error:
            pass
        else:
            raise AssertionError(f"refresh={bad!r} did not raise {error.__name__}")

    # numpy integer scalars are useful in programmatic configurations and
    # satisfy Python's integer-index protocol.
    with contextlib.redirect_stdout(io.StringIO()):
        fit = m.sample(chains=1, warmup=0, samples=1,
                       refresh=np.int64(0))
    assert fit.n_draws == 1


def test_fit_uses_full_transition_report_counts():
    # Stored sampler rows can be thinned and can include saved warmup rows.
    # Fit's headline counts must therefore use the run report, not recount the
    # retained stats array.
    reports = (stanli._SampleReport * 2)()
    reports[0].warmup_seconds = 0.25
    reports[0].sampling_seconds = 0.50
    reports[0].n_divergent = 3
    reports[0].n_max_treedepth = 4
    reports[1].warmup_seconds = 0.75
    reports[1].sampling_seconds = 1.00
    reports[1].n_divergent = 5
    reports[1].n_max_treedepth = 6
    fit = stanli.Fit(["x"], np.zeros((2, 1, 1)), np.zeros((2, 1, 7)),
                     10, 1, reports)
    assert fit.divergences.tolist() == [3, 5]
    assert fit.max_treedepth_hits.tolist() == [4, 6]

    output = io.StringIO()
    with contextlib.redirect_stdout(output):
        stanli._print_sample_reports(reports, first_chain=1, samples=10,
                                     max_depth=12)
    text = output.getvalue()
    assert "8 of 20 post-warmup transitions (40.0%)" in text
    assert "10 of 20 post-warmup transitions (50.0%)" in text
    assert "maximum treedepth of 12" in text
    assert text.count("Elapsed Time:") == 2


def test_sample_opts_defaults_match_the_header():
    # The ctypes struct mirrors stanli_sample_opts field by field, and a
    # mismatch there is a silent misread: the run would sample happily
    # from a scrambled configuration. Asking the C side to fill in its own
    # defaults and checking what Python reads back is what catches a
    # field added on one side only.
    import ctypes
    o = stanli._SampleOpts()
    stanli._lib.stanli_sample_opts_init(ctypes.byref(o))
    assert (o.seed, o.chains, o.chain_id) == (1, 4, 1), (o.seed, o.chains)
    assert (o.warmup, o.samples, o.thin) == (1000, 1000, 1)
    assert o.delta == 0.8 and o.max_depth == 10
    assert o.save_warmup == 0 and o.init_radius == 2.0
    assert o.num_threads == 1 and not o.inits


def test_multichain_shapes_and_dict_protocol():
    m = _es()
    fit = m.sample(chains=3, seed=2, warmup=300, samples=200, refresh=0)
    assert fit.n_chains == 3 and fit.n_draws == 200
    assert fit.draws().shape == (3, 200, len(fit.names))
    assert fit.draws("mu").shape == (3, 200)
    # Indexing by name concatenates the chains, which is what the
    # pre-chains API returned and what an estimate wants.
    assert fit["mu"].shape == (600,)
    assert set(fit.keys()) == set(fit.names)
    assert "mu" in fit and len(fit) == len(fit.names)
    assert dict(fit.items())["mu"].shape == (600,)
    # Sampler columns are reachable by name too.
    assert fit["lp__"].shape == (600,)
    assert fit.sampler_stats.shape == (3, 200, len(stanli.SAMPLER_COLUMNS))


def test_chains_are_different_streams_and_seeds_reproduce():
    m = _es()
    a = m.sample(chains=2, seed=3, warmup=200, samples=100, refresh=0)
    b = m.sample(chains=2, seed=3, warmup=200, samples=100, refresh=0)
    assert (a.draws("mu") == b.draws("mu")).all(), "same seed must reproduce"
    # Chain 2 uses a different stream of the same seed, so its draws must
    # not be chain 1's. Identical chains would mean the chain id never
    # reached create_rng, which no summary statistic would reveal --
    # R-hat of two identical chains is a clean 1.0.
    assert not (a.draws("mu")[0] == a.draws("mu")[1]).all()


def test_summary_and_diagnostics():
    m = _es()
    fit = m.sample(chains=4, seed=4, warmup=1000, samples=1000, refresh=0)
    s = fit.summary()
    assert len(s) == len(fit.names)
    assert 3.0 < s["mu"]["mean"] < 6.0, s["mu"]
    # Rank-normalized split-R-hat on a converged eight schools.
    assert s.r_hat.max() < 1.05, s.r_hat.max()
    assert s.ess_bulk.min() > 100, s.ess_bulk.min()
    assert "R_hat" in str(s) and "mu" in str(s)

    # A converged run must say so in words, not just in numbers. Not "no
    # problems at all": even non-centered eight schools throws the odd
    # divergence, and a test that demanded zero would be fishing for a
    # seed rather than checking the diagnostic. What must hold is that
    # each check reports its PASSING form.
    text = fit.diagnose()
    assert "R-hat is below" in text, text
    assert "Bulk ESS is at least" in text, text
    assert "Tail ESS is at least" in text, text
    assert "E-BFMI is above" in text, text
    assert "saturated the maximum treedepth" in text, text
    assert fit.divergences.shape == (4,)
    assert fit.divergences.sum() < 40, fit.divergences
    assert (fit.stepsize > 0).all()
    ebfmi = fit.ebfmi()
    assert ebfmi.shape == (4,) and (ebfmi > 0.3).all(), ebfmi

    # A single parameter's summary is the same numbers, not a re-fit.
    one = fit.summary(params=["mu"])
    assert len(one) == 1 and one.names == ["mu"]
    assert one["mu"]["mean"] == s["mu"]["mean"]


def test_diagnostics_report_a_broken_fit():
    # A funnel sampled with a loose target and short warmup diverges
    # reliably. The point is not the model, it is that the checks come
    # back with a complaint rather than silence.
    m = stanli.Model(stan_code="""
        parameters { real log_tau; vector[9] z; }
        model { log_tau ~ normal(0, 3); z ~ normal(0, exp(log_tau)); }
    """)
    fit = m.sample(chains=2, seed=5, warmup=150, samples=300, delta=0.5,
                   refresh=0)
    text = fit.diagnose()
    assert ("diverged" in text or "R-hat reaches" in text
            or "ESS falls" in text or "E-BFMI is below" in text), text
    assert "diagnostic check" in text, text


def test_parallel_chains_match_sequential_bitwise():
    # Threading must not be a different answer, only a faster one. Each
    # chain owns its executor and its RNG stream, so the draws are
    # bitwise identical -- and on a build without thread support both
    # runs are sequential and this still holds, which is the point: the
    # assertion cannot be weakened by the build.
    m = _es()
    seq = m.sample(chains=4, seed=11, warmup=300, samples=300,
                   parallel_chains=1, refresh=0)
    par = m.sample(chains=4, seed=11, warmup=300, samples=300,
                   parallel_chains=4, refresh=0)
    assert (seq.draws() == par.draws()).all(), "threading changed the draws"
    assert (seq.sampler_stats == par.sampler_stats).all()


def test_thin_and_save_warmup_change_the_row_count():
    m = _es()
    assert m.sample(chains=1, seed=6, warmup=100, samples=100,
                    thin=4, refresh=0).n_draws == 25
    assert m.sample(chains=1, seed=6, warmup=100, samples=100,
                    save_warmup=True, refresh=0).n_draws == 200


def test_explicit_inits():
    m = _es()
    n = m.n_unconstrained
    # One vector is broadcast to every chain; a per-chain matrix is taken
    # as given. Both must sample.
    assert m.sample(chains=2, seed=8, warmup=50, samples=50,
                    inits=np.zeros(n), refresh=0).n_draws == 50
    assert m.sample(chains=2, seed=8, warmup=50, samples=50,
                    inits=np.zeros((2, n)), refresh=0).n_draws == 50
    # init_radius=0 is CmdStan's `init=0`.
    assert m.sample(chains=1, seed=8, warmup=50, samples=50,
                    init_radius=0.0, refresh=0).n_draws == 50
    try:
        m.sample(chains=2, seed=8, warmup=10, samples=10,
                 inits=np.zeros((3, n)))
    except ValueError:
        pass
    else:
        raise AssertionError("expected ValueError for mismatched inits")


def test_generated_quantities_differ_across_chains():
    # Each chain must get its own RNG stream, or every chain reports the
    # same posterior-predictive draws and the predictive check is a lie.
    code = (FIXTURES / "gqrng.stan").read_text()
    fit = stanli.Model(stan_code=code, data={"N": 5}).sample(
        chains=2, seed=9, warmup=200, samples=50, refresh=0)
    assert not (fit.draws("yrep")[0] == fit.draws("yrep")[1]).all()


def test_optimize_finds_a_mode_and_feeds_sample():
    m = _es()
    r = m.optimize(seed=1)
    # Every CSV column, the way one draw of sample() comes back.
    assert "mu" in r and "tau" in r and "theta.1" in r
    assert r.unconstrained.shape == (m.n_unconstrained,)
    # The reported lp must be the model's lp at that point, not the
    # objective the optimizer minimizes -- that sign slip is silent.
    lp, _ = m.log_prob_grad(r.unconstrained)
    assert abs(lp - r.lp) < 1e-8, (lp, r.lp)
    # And no nearby point may beat it, which is what "mode" means.
    rng = np.random.default_rng(0)
    for _ in range(20):
        q = r.unconstrained + 0.01 * rng.standard_normal(m.n_unconstrained)
        assert m.log_prob_grad(q)[0] <= r.lp + 1e-8

    # Starting the sampler from the mode is the point of having it.
    fit = m.sample(chains=1, seed=2, warmup=200, samples=200,
                   inits=r.unconstrained, refresh=0)
    assert fit.n_draws == 200


def test_optimize_refuses_the_jacobian_free_form():
    # CmdStan's default is jacobian=0; stanli cannot express it, and must
    # say so rather than return the posterior mode under that name.
    m = _es()
    try:
        m.optimize(seed=1, jacobian=False)
    except NotImplementedError as e:
        assert "Jacobian" in str(e)
        return
    raise AssertionError("expected NotImplementedError for jacobian=False")


def test_wrong_size_raises():
    m = stanli.Model(
        stan_code="parameters { real x; } model { x ~ normal(0, 1); }")
    try:
        m.log_prob_grad(np.zeros(3))
    except ValueError:
        return
    raise AssertionError("expected ValueError for a wrong-sized point")


def test_stan_to_mir_round_trips():
    # Compiling a model is two steps -- source to transformed MIR, MIR to
    # an executable graph -- and only the second was reachable without
    # also building a model. A caller that wants to keep the MIR (to cache
    # it, ship it, or hand it to a second process) needs the first step on
    # its own, and what it gets back has to be the same text the in-process
    # path uses: the model built from it must be the same model.
    code = "parameters { real x; } model { x ~ normal(0, 1); }"
    mir = stanli.stan_to_mir(code)
    assert mir.startswith("STANLI2:") or "log_prob" in mir
    suffix = ".exe" if sys.platform == "win32" else ""
    if (stanli._BIN / ("stanli-compile" + suffix)).is_file():
        _portable_payload(mir)
        assert not mir.endswith(("\n", "\r")), "portable MIR has final newline"

    from_source = stanli.Model(stan_code=code)
    from_mir = stanli.Model(mir=mir)
    q = np.array([0.375])
    lp_a, g_a = from_source.log_prob_grad(q)
    lp_b, g_b = from_mir.log_prob_grad(q)
    assert lp_a == lp_b, f"log density differs: {lp_a} vs {lp_b}"
    assert (g_a == g_b).all(), f"gradient differs: {g_a} vs {g_b}"


def test_full_span_assignment_from_source():
    code = """
    data { int<lower=0> N; vector[N] x; }
    transformed data { vector[N] y; y[:] = x; }
    parameters { real z; }
    model { z ~ normal(sum(y), 1); }
    """
    model = stanli.Model(stan_code=code, data={"N": 3, "x": [1, 2, 3]})
    lp, gradient = model.log_prob_grad(np.array([0.0]))
    assert np.isfinite(lp)
    assert gradient.tolist() == [6.0]


def test_stan_to_mir_is_optimized():
    # The MIR handed to the runtime uses stanli's shared optimization policy,
    # not raw transformed MIR. Partial evaluation is the observable here: at
    # O1 stanc3 rewrites log(1 - theta) to log1m(theta), and the transformed
    # MIR never contains log1m. Both compile paths -- the embedded compiler
    # and the subprocess fallback -- must agree on this.
    code = ("parameters { real<lower=0, upper=1> theta; } "
            "model { target += log(1 - theta); }")
    mir = stanli.stan_to_mir(code)
    optimized = (_portable_payload(mir) if mir.startswith("STANLI2:")
                 else mir.encode("utf-8"))
    assert b"log1m" in optimized, "MIR is not optimized (expected log1m from --O1)"


def test_stan_to_mir_reports_syntax_errors():
    try:
        stanli.stan_to_mir("parameters { real x } model { }")
    except RuntimeError:
        return
    raise AssertionError("expected RuntimeError for a syntax error")


def test_subprocess_compiler_preference_and_rollback_contract():
    # The Windows wheel carries both executables for one release cycle. The
    # portable producer wins by presence; a non-zero result from it is a real
    # failure and must never be hidden by silently retrying pristine stanc.
    suffix = ".exe" if sys.platform == "win32" else ""
    with tempfile.TemporaryDirectory() as tmpdir:
        compiler_dir = pathlib.Path(tmpdir)
        portable = compiler_dir / ("stanli-compile" + suffix)
        stock = compiler_dir / ("stanc" + suffix)
        source = compiler_dir / "source with spaces.stan"
        portable.touch()
        stock.touch()
        source.write_text("model {}", encoding="utf-8")
        calls = []

        def successful_run(argv, **kwargs):
            calls.append((argv, kwargs))
            return subprocess.CompletedProcess(
                argv, 0, "STANLI2:ZmFrZQ==",
                "a successful frontend warning")

        with mock.patch.object(stanli, "_BIN", compiler_dir), \
                mock.patch.object(stanli.subprocess, "run", successful_run):
            got = stanli._stanc_mir(source)
        assert got == "STANLI2:ZmFrZQ=="
        assert calls[0][0] == [str(portable), str(source)]
        assert calls[0][1]["encoding"] == "utf-8"
        assert calls[0][1]["capture_output"] is True

        calls.clear()

        def failing_run(argv, **kwargs):
            calls.append(argv)
            return subprocess.CompletedProcess(argv, 1, "partial", "bad Stan")

        with mock.patch.object(stanli, "_BIN", compiler_dir), \
                mock.patch.object(stanli.subprocess, "run", failing_run):
            try:
                stanli._stanc_mir(source)
            except RuntimeError as exc:
                assert "bad Stan" in str(exc)
            else:
                raise AssertionError("preferred compiler failure was ignored")
        assert calls == [[str(portable), str(source)]], calls

        portable.unlink()
        calls.clear()
        with mock.patch.object(stanli, "_BIN", compiler_dir), \
                mock.patch.object(stanli.subprocess, "run", successful_run):
            stanli._stanc_mir(source)
        assert calls[0][0] == [str(stock), "--O1",
                               "--debug-optimized-mir", str(source)]

        stock.unlink()
        with mock.patch.object(stanli, "_BIN", compiler_dir):
            try:
                stanli._compiler_command(source)
            except RuntimeError as exc:
                assert "stanli-compile" in str(exc) and "stanc" in str(exc)
            else:
                raise AssertionError("missing bundled compilers were accepted")


def test_subprocess_source_is_exact_utf8_and_fully_cleaned_up():
    code = ("parameters { real theta; }\r\n"
            "model { // θ\r\n theta ~ normal(0, 1); }\r\n")
    seen = {}

    def fake_compile(source):
        seen["source"] = source
        seen["bytes"] = source.read_bytes()
        # Pristine stanc writes this beside its input even when only its debug
        # MIR was requested. The whole private directory must be reclaimed.
        source.with_suffix(".hpp").write_text("generated", encoding="utf-8")
        return "(fake MIR)"

    with mock.patch.object(stanli, "_stanc_mir", fake_compile):
        assert stanli._subprocess_mir(code) == "(fake MIR)"
    assert seen["bytes"] == code.encode("utf-8")
    assert not seen["source"].parent.exists()

    with tempfile.TemporaryDirectory() as tmpdir:
        source_file = pathlib.Path(tmpdir) / "source π.stan"
        source_file.write_bytes(code.encode("utf-8"))
        assert stanli._read_utf8_file(source_file) == code


def test_windows_wheel_compilers_and_stock_rollback_execute():
    if sys.platform != "win32":
        return

    packaged_portable = stanli._BIN / "stanli-compile.exe"
    packaged_stock = stanli._BIN / "stanc.exe"
    assert packaged_portable.is_file(), "Windows wheel lacks stanli-compile.exe"
    assert packaged_stock.is_file(), "Windows wheel lacks stanc.exe"

    unicode_text = "π ☃ é 👋"
    code = ("transformed data {\r\n"
            f'  print("{unicode_text}");\r\n'
            "}\r\n"
            "parameters { real x; }\r\n"
            "model { x ~ normal(0, 1); }\r\n")
    with tempfile.TemporaryDirectory(
            prefix="stanli compiler rollback π ") as tmp:
        compiler_dir = pathlib.Path(tmp)
        portable = compiler_dir / packaged_portable.name
        stock = compiler_dir / packaged_stock.name
        shutil.copy2(packaged_portable, portable)
        shutil.copy2(packaged_stock, stock)

        with mock.patch.object(stanli, "_BIN", compiler_dir):
            portable_mir = stanli._subprocess_mir(code)
            payload = _portable_payload(portable_mir)
            assert not portable_mir.endswith(("\n", "\r"))
            assert unicode_text.encode("utf-8") in payload

            # Removing only the temporary preferred producer must select the
            # packaged rollback compiler. Its legacy MIR still has to lower
            # into the same executable runtime graph.
            portable.unlink()
            legacy_mir = stanli._subprocess_mir(code)

        assert legacy_mir.lstrip().startswith("(")
        model = stanli.Model(mir=legacy_mir)
        lp, grad = model.log_prob_grad(np.array([0.25]))
        assert np.isfinite(lp)
        assert np.isfinite(grad).all()


def test_embedded_stanc_from_concurrent_python_threads():
    # ctypes releases the GIL around C calls.  Initialize the embedded OCaml
    # runtime here, then make several foreign Python threads enter it at once.
    # Each worker compiles twice so it also exercises unregistering and
    # re-registering the same C thread with OCaml's main domain.
    if not stanli._lib.stanli_has_embedded_stanc():
        return

    code = "parameters { real x; } model { x ~ normal(0, 1); }"
    expected = stanli.stan_to_mir(code)
    _portable_payload(expected)

    n_workers = 4
    ready = threading.Barrier(n_workers)

    def compile_twice(worker):
        ready.wait()
        first = stanli.stan_to_mir(code)
        second = stanli.stan_to_mir(code)
        error = None
        if worker == 0:
            try:
                stanli.stan_to_mir("parameters { real x } model { }")
            except RuntimeError as exc:
                error = str(exc)
            else:
                raise AssertionError("foreign-thread syntax error succeeded")
        return first, second, error

    with concurrent.futures.ThreadPoolExecutor(
            max_workers=n_workers) as executor:
        results = list(executor.map(compile_twice, range(n_workers)))

    for worker, (first, second, error) in enumerate(results):
        assert first == expected
        assert second == expected
        assert bool(error) == (worker == 0)


def test_build_id_is_stable_and_specific():
    # Identifies the runtime binary. Anything caching artifacts keyed to a
    # build (a compiled MIR next to the library that produced it) needs a
    # value that changes when the library does and not otherwise.
    a = stanli.build_id()
    assert isinstance(a, str) and a, "build id is empty"
    assert a == stanli.build_id(), "build id is not stable within a process"


def main():
    failed = 0
    for name, fn in sorted(globals().items()):
        if not name.startswith("test_"):
            continue
        try:
            fn()
            print(f"ok   {name}")
        except Exception as e:  # noqa: BLE001 - report and count everything
            failed += 1
            print(f"FAIL {name}: {e}")
    print(f"\n{'FAILED' if failed else 'all passed'}")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
