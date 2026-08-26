# Exercise the candidate R/webR compiler without changing the compiler bundled
# in the R package. The candidate is the exact browser-compiler artifact from
# this workflow; its output is handed to the public MIR entry point of an
# installed package backed by the real Linux runtime.
#
# Usage: Rscript tests/test_r_portable_candidate.R path/to/stanli-compiler.js

args <- commandArgs(trailingOnly = TRUE)
if (length(args) != 1L)
  stop("usage: test_r_portable_candidate.R <stanli-compiler.js>",
       call. = FALSE)

compiler <- normalizePath(args[[1]], mustWork = TRUE)
if (!requireNamespace("V8", quietly = TRUE) ||
    !requireNamespace("jsonlite", quietly = TRUE))
  stop("the candidate check requires V8 and jsonlite", call. = FALSE)

# Deliberately do not use stanli's compiler helpers here. This is a fresh V8
# context and an explicit call to the candidate's stanli_compile() export, so
# the check cannot accidentally pass through the compiler bundled in the R
# package.
ctx <- V8::v8()
ctx$source(compiler)

compile_candidate <- function(name, code) {
  ctx$assign("stanli_candidate_name", name)
  ctx$assign("stanli_candidate_source", enc2utf8(code))
  payload <- ctx$eval(
    "(function () {
       var f = globalThis.stanli_compile;
       if (typeof f !== 'function')
         return JSON.stringify({internal_error: 'no stanli_compile()'});
       var r = f(stanli_candidate_name, stanli_candidate_source);
       function strings(xs) {
         return xs ? Array.prototype.map.call(xs, String) : [];
       }
       return JSON.stringify({
         result: (typeof r.result === 'undefined') ? null : String(r.result),
         errors: strings(r.errors),
         warnings: strings(r.warnings)
       });
     })()")
  out <- jsonlite::fromJSON(payload, simplifyVector = TRUE)
  if (!is.null(out$internal_error)) stop(out$internal_error, call. = FALSE)
  out
}

unicode_source <- enc2utf8("\
transformed data { print(\"pi: π, snowman: ☃, wave: 👋\"); }
parameters { real mu; }
model { mu ~ std_normal(); }")
first <- compile_candidate("unicode_candidate", unicode_source)
second <- compile_candidate("unicode_candidate", unicode_source)
stopifnot(
  length(first$errors) == 0L,
  is.character(first$result), length(first$result) == 1L,
  startsWith(first$result, '{"stanli_ir":1,"program":'),
  !grepl("[\r\n]$", first$result),
  validUTF8(first$result),
  grepl("π", first$result, fixed = TRUE),
  identical(first$result, second$result),
  identical(first$warnings, second$warnings)
)

warning_source <- "
parameters { real x; }
model { if (0 < x < 1) target += 0; }"
warning_result <- compile_candidate("warning_candidate", warning_source)
stopifnot(length(warning_result$errors) == 0L,
          length(warning_result$warnings) > 0L)

bad <- compile_candidate(
  "malformed_candidate", "parameters { real x } model {}")
stopifnot(is.null(bad$result), length(bad$errors) > 0L)

suppressPackageStartupMessages(library(stanli))
if (!stanli_available())
  stop("the real stanli runtime is required for this check", call. = FALSE)

runtime_source <- "
parameters { real mu; }
model { mu ~ std_normal(); }
generated quantities {
  real mu_twice = 2 * mu;
  real y_rep = normal_rng(mu, 1);
}"
runtime_compilation <- compile_candidate("runtime_candidate", runtime_source)
stopifnot(length(runtime_compilation$errors) == 0L,
          startsWith(runtime_compilation$result,
                     '{"stanli_ir":1,"program":'))

# `mir` is the public compatibility seam. Supplying it forces this exact
# candidate document through the dual decoder even though the Linux runtime
# also contains an embedded source compiler.
model <- stanli_model(mir = runtime_compilation$result)
gradient <- log_prob_grad(model, 0.25)
stopifnot(is.finite(gradient$lp),
          length(gradient$grad) == 1L,
          isTRUE(all.equal(unname(gradient$grad), -0.25,
                           tolerance = 1e-12)))

fit <- sample_model(model, chains = 1L, seed = 9182L, warmup = 20L,
                    samples = 12L, init = 0, init_radius = 0,
                    parallel_chains = 1L)
stopifnot(all(c("mu", "mu_twice", "y_rep") %in% fit$columns),
          all(is.finite(fit$draws[, , "y_rep"])),
          isTRUE(all.equal(as.numeric(fit$draws[, , "mu_twice"]),
                           2 * as.numeric(fit$draws[, , "mu"]),
                           tolerance = 1e-12)))

message("R portable candidate OK: V8, decoder, gradient, sampling, and GQ")
