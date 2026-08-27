# Exercise the candidate R/webR compiler without changing the compiler bundled
# in the R package. The candidate is the exact browser-compiler artifact from
# this workflow. It is installed into the R package's real V8 helper, and that
# helper's output is handed to the public MIR entry point of an installed
# package backed by the real Linux runtime.
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

suppressPackageStartupMessages(library(stanli))

# Put the exact candidate artifact in the context cached by mir_from_js(). The
# package still carries its tracked legacy file; this test changes only its own
# fresh R session. Wrap both exports so each helper call proves which producer
# ran and records the portable warnings that the source-compilation API leaves
# internal on success.
ctx <- V8::v8()
ctx$source(compiler)
ctx$eval("(function () {
  var portable = globalThis.stanli_compile;
  var classic = globalThis.stanc;
  if (typeof portable !== 'function')
    throw new Error('no stanli_compile() export');
  if (typeof classic !== 'function') throw new Error('no stanc() export');
  globalThis.__stanli_candidate = {
    portable_calls: 0, classic_calls: 0, warnings: []
  };
  globalThis.stanli_compile = function() {
    globalThis.__stanli_candidate.portable_calls += 1;
    var r = portable.apply(this, arguments);
    globalThis.__stanli_candidate.warnings = r.warnings
      ? Array.prototype.map.call(r.warnings, String) : [];
    return r;
  };
  globalThis.stanc = function() {
    globalThis.__stanli_candidate.classic_calls += 1;
    return classic.apply(this, arguments);
  };
})()")

compiler_state <- function() {
  jsonlite::fromJSON(
    ctx$eval("JSON.stringify(globalThis.__stanli_candidate)"),
    simplifyVector = TRUE)
}

state <- getFromNamespace("stanc_js_ctx", "stanli")
state$ctx <- ctx

compile_candidate <- function(name, code) {
  before <- compiler_state()
  error <- NULL
  result <- tryCatch(
    stanli:::mir_from_js(enc2utf8(code), enc2utf8(name)),
    error = function(e) {
      error <<- conditionMessage(e)
      NULL
    })
  after <- compiler_state()
  if (!identical(after$portable_calls, before$portable_calls + 1L) ||
      !identical(after$classic_calls, before$classic_calls))
    stop("the R helper did not use exactly one portable compiler call",
         call. = FALSE)
  list(result = result,
       errors = if (is.null(error)) character() else error,
       warnings = after$warnings)
}

portable_payload <- function(mir) {
  stopifnot(startsWith(mir, "STANLI2:"))
  jsonlite::base64_dec(substring(mir, 9L))
}

raw_contains <- function(haystack, text) {
  needle <- charToRaw(enc2utf8(text))
  if (length(needle) == 0L) return(TRUE)
  if (length(haystack) < length(needle)) return(FALSE)
  any(vapply(seq_len(length(haystack) - length(needle) + 1L), function(i) {
    identical(haystack[i:(i + length(needle) - 1L)], needle)
  }, logical(1)))
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
  startsWith(first$result, "STANLI2:"),
  !grepl("[\r\n]$", first$result),
  validUTF8(first$result),
  raw_contains(portable_payload(first$result), "π"),
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
stopifnot(is.null(bad$result), length(bad$errors) > 0L,
          grepl("stanli_compile", bad$errors, fixed = TRUE),
          compiler_state()$classic_calls == 0L)

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
          startsWith(runtime_compilation$result, "STANLI2:"))

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
