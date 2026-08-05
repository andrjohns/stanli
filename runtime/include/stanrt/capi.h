/* stanrt C ABI: the stable boundary language bindings speak to.
 * Exception-free; every entry returns an error code or null on failure and
 * writes a message into the caller's buffer. One stanrt_model per (model,
 * data) pair; not thread-safe per instance (use one per chain). */
#ifndef STANRT_CAPI_H
#define STANRT_CAPI_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct stanrt_model stanrt_model;

/* Compile transformed-MIR sexp text (from `stanc --debug-transformed-mir`)
 * against JSON data (CmdStan conventions). Returns null on failure with a
 * message in err. */
stanrt_model* stanrt_model_new(const char* tmir_sexp, const char* data_json,
                               char* err, size_t err_len);
void stanrt_model_free(stanrt_model* m);

int64_t stanrt_n_unconstrained(const stanrt_model* m);

/* log_prob (propto=false, jacobian included) and its gradient at
 * unconstrained q[n]. grad may be null for value-only. Returns 0 on success,
 * 1 on a rejected evaluation (domain error; *lp set to -inf). */
int stanrt_grad(stanrt_model* m, const double* q, double* lp, double* grad);

/* NUTS with diagonal-metric adaptation. draws must hold
 * samples * n_unconstrained doubles (row-major, one draw per row).
 * Returns 0 on success, nonzero with message in err otherwise. */
int stanrt_sample(stanrt_model* m, uint32_t seed, int warmup, int samples,
                  double delta, double* draws, char* err, size_t err_len);

/* Constrained view: flattened parameter values for one unconstrained q.
 * n_constrained gives the output length; names are "mu", "theta.1", ... */
int64_t stanrt_n_constrained(const stanrt_model* m);
const char* stanrt_constrained_name(const stanrt_model* m, int64_t i);
int stanrt_constrain(stanrt_model* m, const double* q, double* out);

#ifdef __cplusplus
}
#endif

#endif
