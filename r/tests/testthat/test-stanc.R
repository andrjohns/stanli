# The one path that works without a runtime, and so the only thing CRAN's
# check farm will actually execute: stanc3 compiled to JavaScript, run
# through V8. It is what makes the package shippable there at all, so it
# is worth a test that does not skip on the machines that matter.

test_that("the bundled JavaScript compiler produces MIR", {
  skip_if_not_installed("V8")
  skip_if_not_installed("jsonlite")
  skip_if(!nzchar(stanli:::stanc_js_path()) ||
            !file.exists(stanli:::stanc_js_path()),
          "stanc.js is not in this installation")

  mir <- stanli:::mir_from_js("
    data { int<lower=0> N; vector[N] y; }
    parameters { real mu; real<lower=0> sigma; }
    model { y ~ normal(mu, sigma); }")

  expect_type(mir, "character")
  # An s-expression naming the model's own symbols, not just any output:
  # a compiler that silently emitted an empty program would still be a
  # non-empty string.
  expect_match(mir, "\\bmu\\b")
  expect_match(mir, "\\bsigma\\b")
  expect_match(mir, "normal")
})

test_that("the bundled JavaScript compiler applies O1", {
  skip_if_not_installed("V8")
  skip_if_not_installed("jsonlite")
  skip_if(!nzchar(stanli:::stanc_js_path()) ||
            !file.exists(stanli:::stanc_js_path()),
          "stanc.js is not in this installation")

  mir <- stanli:::mir_from_js("
    parameters { real theta; }
    model {
      target += 0.1 + 0.2;
      theta ~ std_normal();
    }")

  expect_match(mir, "(Lit Real 0.30000000000000004)", fixed = TRUE)
})

test_that("the native portable compiler owns flags and keeps warnings separate", {
  seen <- NULL
  run_stanc <- function(compiler, args, stdout, stderr) {
    seen <<- list(compiler = compiler, args = args,
                  stdout = stdout, stderr = stderr,
                  work = dirname(stdout))
    source <- file.path(dirname(stdout), "model.stan")
    source_bytes <- readBin(source, "raw", n = file.info(source)$size)
    expect_identical(source_bytes,
                     charToRaw(enc2utf8("model { // θ\r\n}\r\n")))
    con <- file(stdout, open = "wb")
    writeChar(enc2utf8('{"stanli_ir":1,"program":"theta θ"}'), con,
              eos = NULL, useBytes = TRUE)
    close(con)
    writeLines("a successful frontend warning", stderr, useBytes = TRUE)
    0L
  }

  expect_identical(stanli:::mir_from_binary(
                     "fake-stanli-compile", "model { // θ\r\n}\r\n",
                     portable = TRUE,
                     run_stanc = run_stanc),
                   '{"stanli_ir":1,"program":"theta θ"}')
  expect_identical(seen$compiler, "fake-stanli-compile")
  expect_length(seen$args, 1)
  expect_false(dir.exists(seen$work))
})

test_that("UTF-8 Stan files survive locale-independent compiler staging", {
  source_file <- tempfile(fileext = ".stan")
  on.exit(unlink(source_file), add = TRUE)
  source_text <- enc2utf8("transformed data { print(\"\u03b8\"); }\nmodel {}\n")
  writeBin(charToRaw(source_text), source_file)

  old_locale <- Sys.getlocale("LC_CTYPE")
  on.exit(suppressWarnings(Sys.setlocale("LC_CTYPE", old_locale)), add = TRUE)
  expect_true(nzchar(suppressWarnings(Sys.setlocale("LC_CTYPE", "C"))))

  code <- stanli:::read_utf8_file(source_file)
  expected <- charToRaw(source_text)
  expect_identical(charToRaw(code), expected)

  run_stanc <- function(compiler, args, stdout, stderr) {
    staged <- file.path(dirname(stdout), "model.stan")
    expect_identical(readBin(staged, "raw", n = file.info(staged)$size),
                     expected)
    writeChar('{"stanli_ir":1,"program":{}}', stdout, eos = NULL,
              useBytes = TRUE)
    file.create(stderr)
    0L
  }
  expect_identical(
    stanli:::mir_from_binary("fake-stanli-compile", code, portable = TRUE,
                              run_stanc = run_stanc),
    '{"stanli_ir":1,"program":{}}')
})

test_that("the pristine stanc fallback requests O1 optimized MIR", {
  seen <- NULL
  run_stanc <- function(compiler, args, stdout, stderr) {
    seen <<- list(compiler = compiler, args = args, work = dirname(stdout))
    writeLines("(fake MIR)", stdout, useBytes = TRUE)
    writeLines("generated side effect", file.path(dirname(stdout), "model.hpp"))
    file.create(stderr)
    0L
  }

  expect_identical(stanli:::mir_from_binary(
                     "fake-stanc", "model {}", run_stanc = run_stanc),
                   "(fake MIR)")
  expect_identical(seen$compiler, "fake-stanc")
  expect_identical(seen$args[1:2],
                   c("--O1", "--debug-optimized-mir"))
  expect_false(dir.exists(seen$work))
})

test_that("a failing preferred compiler is not hidden by partial output", {
  run_stanc <- function(compiler, args, stdout, stderr) {
    writeLines("partial portable output", stdout)
    writeLines("source-bearing syntax error", stderr)
    1L
  }

  expect_error(
    stanli:::mir_from_binary("fake-stanli-compile", "bad model",
                              portable = TRUE, run_stanc = run_stanc),
    "source-bearing syntax error")
})

test_that("native discovery prefers the packaged portable compiler", {
  work <- tempfile("stanli-compiler-discovery-")
  dir.create(work)
  on.exit(unlink(work, recursive = TRUE), add = TRUE)
  runtime <- file.path(work, "stanli.dll")
  portable <- file.path(work, "stanli-compile.exe")
  stanc <- file.path(work, "stanc.exe")
  file.create(runtime, portable, stanc)

  from_path <- function(name) {
    expect_identical(name, "stanc.exe")
    "C:/tools/stanc.exe"
  }
  expect_identical(
    stanli:::find_native_compiler("Windows", runtime, "", from_path),
    list(path = portable, portable = TRUE))

  # Explicit configuration remains the bisect/override mechanism.
  expect_identical(
    stanli:::find_native_compiler("Windows", runtime, "C:/old/stanc.exe",
                                   from_path),
    list(path = "C:/old/stanc.exe", portable = FALSE))

  unlink(portable)
  expect_identical(
    stanli:::find_native_compiler("Windows", runtime, "", from_path),
    list(path = stanc, portable = FALSE))
  unlink(stanc)
  expect_identical(
    stanli:::find_native_compiler("Windows", runtime, "", from_path),
    list(path = "C:/tools/stanc.exe", portable = FALSE))

  expect_null(stanli:::find_native_compiler(
    "Emscripten", runtime, "",
    function(name) stop("webR must not search for a process")))
})

test_that("the webR compiler fallback requests O1 optimized MIR", {
  state <- stanli:::stanc_js_ctx
  had_loaded <- exists("webr_loaded", envir = state, inherits = FALSE)
  old_loaded <- state$webr_loaded
  on.exit(if (had_loaded) state$webr_loaded <- old_loaded else
            rm("webr_loaded", envir = state), add = TRUE)
  if (had_loaded) rm("webr_loaded", envir = state)

  compile_script <- NULL
  eval_js <- function(script) {
    if (!grepl("const src = Module.FS.readFile", script, fixed = TRUE))
      return("compiler loaded")
    compile_script <<- script
    match <- regexec("Module\\.FS\\.writeFile\\('([^']+)'", script)
    groups <- regmatches(script, match)[[1]]
    expect_length(groups, 2)
    writeLines("(fake MIR)", groups[[2]])
    "ok"
  }

  expect_identical(stanli:::mir_from_webr(eval_js, "model {}"), "(fake MIR)")
  expect_match(compile_script, "['O1', 'debug-optimized-mir']", fixed = TRUE)
})

test_that("a model that does not typecheck is an error, not empty MIR", {
  skip_if_not_installed("V8")
  skip_if_not_installed("jsonlite")
  skip_if(!nzchar(stanli:::stanc_js_path()) ||
            !file.exists(stanli:::stanc_js_path()),
          "stanc.js is not in this installation")

  # `y` is undeclared. stanc reports this through `errors` rather than a
  # thrown exception, so the wrapper has to look for it: without that
  # check a broken model reaches the lowering pass as NULL.
  expect_error(
    stanli:::mir_from_js("parameters { real mu; } model { mu ~ normal(y, 1); }"),
    "stanc")
})

test_that("runtime asset names match the five published targets", {
  # r/R/install.R and the release workflow build these names independently.
  # Check the supported targets directly: R CMD check may itself run on an
  # unsupported host, such as Windows arm64.
  published <- c("stanli-runtime-darwin-arm64.tar.gz",
                 "stanli-runtime-darwin-x86_64.tar.gz",
                 "stanli-runtime-linux-x86_64.tar.gz",
                 "stanli-runtime-linux-arm64.tar.gz",
                 "stanli-runtime-windows-x86_64.tar.gz")
  os <- c("darwin", "darwin", "linux", "linux", "windows")
  arch <- c("arm64", "x86_64", "x86_64", "arm64", "x86_64")
  actual <- mapply(stanli:::runtime_asset, os, arch, USE.NAMES = FALSE)

  expect_equal(actual, published)
})
