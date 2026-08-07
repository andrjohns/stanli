#!/usr/bin/env python3
"""Tests for the Python package: the ctypes layer and its error paths.

Run against an installed wheel (CI does) or a local build:

    tools/build_wheel.sh && PYTHONPATH=python python3 tests/test_python.py
"""
import pathlib
import sys

import numpy as np

import stanli

FIXTURES = pathlib.Path(__file__).resolve().parent / "fixtures"


def test_sample_eight_schools():
    m = stanli.Model(stan_file=FIXTURES / "es.stan",
                     data=FIXTURES / "eight_schools.json")
    d = m.sample(seed=1, warmup=1000, samples=1000)
    mu = d["mu"].mean()
    assert 3.0 < mu < 6.0, mu


def test_log_prob_grad():
    m = stanli.Model(stan_file=FIXTURES / "es.stan",
                     data=FIXTURES / "eight_schools.json")
    lp, grad = m.log_prob_grad(np.zeros(m.n_unconstrained))
    assert np.isfinite(lp), lp
    assert grad.shape == (m.n_unconstrained,)
    assert np.isfinite(grad).all(), grad


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
    d = m.sample(seed=1, warmup=200, samples=100)
    for col in ("mu", "tau", "theta.1", "theta.8"):
        assert col in d, sorted(d)[:12]
    assert abs(d["theta.1"].mean()) < 30.0


def test_sample_returns_generated_quantities():
    # RNG draws and draw-dependent branches run through the interpreted
    # write_array; the columns and the seeded RNG stream must both reach
    # Python.
    code = (FIXTURES / "gqrng.stan").read_text()
    m = stanli.Model(stan_code=code, data={"N": 5})
    d = m.sample(seed=7, warmup=200, samples=50)
    for col in ("sigma", "yrep", "crep", "branchy", "p"):
        assert col in d, sorted(d)
    crep = d["crep"]
    assert ((crep == np.floor(crep)) & (crep >= 0) & (crep <= 5)).all()
    assert (d["p"] == 6.0).all()
    assert d["yrep"].std() > 0.0
    d2 = stanli.Model(stan_code=code, data={"N": 5}).sample(
        seed=7, warmup=200, samples=50)
    assert (d["yrep"] == d2["yrep"]).all(), "same seed, different GQ draws"


def test_wrong_size_raises():
    m = stanli.Model(
        stan_code="parameters { real x; } model { x ~ normal(0, 1); }")
    try:
        m.log_prob_grad(np.zeros(3))
    except ValueError:
        return
    raise AssertionError("expected ValueError for a wrong-sized point")


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
