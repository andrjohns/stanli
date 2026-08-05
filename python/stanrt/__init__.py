"""stanrt: portable Stan runtime.

Compiles a .stan model via the bundled stanc3 binary (subprocess), lowers it
in-process through the bundled shared library, and samples with NUTS. No C++
toolchain, no model compilation on this machine.
"""
import ctypes
import json
import pathlib
import subprocess
import sys

import numpy as np

__all__ = ["Model", "__version__"]
__version__ = "0.1.0.dev0"

_BIN = pathlib.Path(__file__).parent / "_bin"


def _load_lib():
    names = {"darwin": "libstanrt.dylib", "linux": "libstanrt.so"}
    lib = ctypes.CDLL(str(_BIN / names.get(sys.platform, "stanrt.dll")))
    lib.stanrt_model_new.restype = ctypes.c_void_p
    lib.stanrt_model_new.argtypes = [ctypes.c_char_p, ctypes.c_char_p,
                                     ctypes.c_char_p, ctypes.c_size_t]
    lib.stanrt_model_free.argtypes = [ctypes.c_void_p]
    lib.stanrt_n_unconstrained.restype = ctypes.c_int64
    lib.stanrt_n_unconstrained.argtypes = [ctypes.c_void_p]
    lib.stanrt_grad.restype = ctypes.c_int
    lib.stanrt_grad.argtypes = [ctypes.c_void_p,
                                ctypes.POINTER(ctypes.c_double),
                                ctypes.POINTER(ctypes.c_double),
                                ctypes.POINTER(ctypes.c_double)]
    lib.stanrt_sample.restype = ctypes.c_int
    lib.stanrt_sample.argtypes = [ctypes.c_void_p, ctypes.c_uint32,
                                  ctypes.c_int, ctypes.c_int, ctypes.c_double,
                                  ctypes.POINTER(ctypes.c_double),
                                  ctypes.c_char_p, ctypes.c_size_t]
    lib.stanrt_n_constrained.restype = ctypes.c_int64
    lib.stanrt_n_constrained.argtypes = [ctypes.c_void_p]
    lib.stanrt_constrained_name.restype = ctypes.c_char_p
    lib.stanrt_constrained_name.argtypes = [ctypes.c_void_p, ctypes.c_int64]
    lib.stanrt_constrain.restype = ctypes.c_int
    lib.stanrt_constrain.argtypes = [ctypes.c_void_p,
                                     ctypes.POINTER(ctypes.c_double),
                                     ctypes.POINTER(ctypes.c_double)]
    return lib


_lib = _load_lib()


def _stanc_mir(model_path: pathlib.Path) -> str:
    stanc = _BIN / ("stanc.exe" if sys.platform == "win32" else "stanc")
    r = subprocess.run([str(stanc), "--debug-transformed-mir",
                        str(model_path)],
                       capture_output=True, text=True)
    if r.returncode != 0 or not r.stdout:
        raise RuntimeError(f"stanc failed:\n{r.stderr}")
    return r.stdout


class Model:
    """A compiled (model, data) pair."""

    def __init__(self, stan_file=None, data=None, stan_code=None):
        if stan_code is not None:
            import tempfile
            tmp = pathlib.Path(tempfile.mkdtemp()) / "model.stan"
            tmp.write_text(stan_code)
            stan_file = tmp
        if stan_file is None:
            raise ValueError("provide stan_file or stan_code")
        if isinstance(data, (str, pathlib.Path)):
            data_json = pathlib.Path(data).read_text()
        else:
            data_json = json.dumps(data or {})

        mir = _stanc_mir(pathlib.Path(stan_file))
        err = ctypes.create_string_buffer(4096)
        self._m = _lib.stanrt_model_new(mir.encode(), data_json.encode(),
                                        err, len(err))
        if not self._m:
            raise RuntimeError(err.value.decode())
        self.n_unconstrained = _lib.stanrt_n_unconstrained(self._m)
        n_con = _lib.stanrt_n_constrained(self._m)
        self.constrained_names = [
            _lib.stanrt_constrained_name(self._m, i).decode()
            for i in range(n_con)
        ]

    def __del__(self):
        if getattr(self, "_m", None):
            _lib.stanrt_model_free(self._m)
            self._m = None

    def log_prob_grad(self, q):
        """log density (jacobian included) and gradient at unconstrained q."""
        q = np.ascontiguousarray(q, dtype=np.float64)
        assert q.size == self.n_unconstrained
        lp = ctypes.c_double()
        grad = np.empty(self.n_unconstrained)
        _lib.stanrt_grad(self._m,
                         q.ctypes.data_as(ctypes.POINTER(ctypes.c_double)),
                         ctypes.byref(lp),
                         grad.ctypes.data_as(ctypes.POINTER(ctypes.c_double)))
        return lp.value, grad

    def sample(self, *, seed=1, warmup=1000, samples=1000, delta=0.8):
        """NUTS draws as {name: array} of constrained parameters."""
        n = self.n_unconstrained
        draws = np.empty((samples, n))
        err = ctypes.create_string_buffer(4096)
        rc = _lib.stanrt_sample(
            self._m, seed, warmup, samples, delta,
            draws.ctypes.data_as(ctypes.POINTER(ctypes.c_double)),
            err, len(err))
        if rc != 0:
            raise RuntimeError(err.value.decode())
        n_con = len(self.constrained_names)
        con = np.empty((samples, n_con))
        row = np.empty(n_con)
        for s in range(samples):
            _lib.stanrt_constrain(
                self._m,
                draws[s].ctypes.data_as(ctypes.POINTER(ctypes.c_double)),
                row.ctypes.data_as(ctypes.POINTER(ctypes.c_double)))
            con[s] = row
        return {name: con[:, i]
                for i, name in enumerate(self.constrained_names)}
