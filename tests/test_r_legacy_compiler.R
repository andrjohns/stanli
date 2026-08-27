# Run the released v0.9.3 R package's legacy JavaScript compiler against the
# runtime built from the current checkout. The caller selects both by putting
# the v0.9.3 package library first in R_LIBS_USER and the new library in
# STANLI_RUNTIME.

args <- commandArgs(trailingOnly = TRUE)
if (length(args) != 0L)
  stop("usage: test_r_legacy_compiler.R", call. = FALSE)

if (!requireNamespace("V8", quietly = TRUE) ||
    !requireNamespace("jsonlite", quietly = TRUE))
  stop("the legacy compiler check requires V8 and jsonlite", call. = FALSE)

suppressPackageStartupMessages(library(stanli))
stopifnot(identical(as.character(utils::packageVersion("stanli")), "0.9.3"))

source <- "
parameters { real mu; }
model { mu ~ std_normal(); }
generated quantities {
  real mu_twice = 2 * mu;
  real y_rep = normal_rng(mu, 1);
}"
mir <- stanli:::mir_from_js(source, "legacy_release")
stopifnot(is.character(mir), length(mir) == 1L,
          startsWith(mir, "((functions_block"),
          !startsWith(mir, "STANLI2:"))

if (!stanli_available())
  stop("the current stanli runtime is required for this check", call. = FALSE)

model <- stanli_model(mir = mir)
gradient <- log_prob_grad(model, 0.25)
stopifnot(is.finite(gradient$lp),
          length(gradient$grad) == 1L,
          isTRUE(all.equal(unname(gradient$grad), -0.25,
                           tolerance = 1e-12)))

fit <- sample_model(model, chains = 1L, seed = 9182L, warmup = 10L,
                    samples = 6L, init = 0, init_radius = 0,
                    parallel_chains = 1L)
stopifnot(all(c("mu", "mu_twice", "y_rep") %in% fit$columns),
          all(is.finite(fit$draws[, , "y_rep"])),
          isTRUE(all.equal(as.numeric(fit$draws[, , "mu_twice"]),
                           2 * as.numeric(fit$draws[, , "mu"]),
                           tolerance = 1e-12)))

message("R v0.9.3 legacy compiler OK against the current runtime")
