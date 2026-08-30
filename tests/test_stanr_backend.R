# Run the downstream package's own Stanli backend suite against the installed
# stanr built by test_stanr.sh. Keeping the tests in stanr means this gate
# follows the real consumer contract rather than duplicating a partial facade.

args <- commandArgs(trailingOnly = TRUE)
if (length(args) != 1L || !dir.exists(args[[1L]]))
  stop("usage: test_stanr_backend.R STANR_SOURCE_DIR", call. = FALSE)

suppressPackageStartupMessages({
  library(stanr)
  library(testthat)
})

testthat::test_dir(
  file.path(args[[1L]], "tests", "testthat"),
  filter = "stanli-backend",
  reporter = "summary",
  stop_on_failure = TRUE
)

message("stanr's Stanli backend suite passed")
