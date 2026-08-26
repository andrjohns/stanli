# Getting transformed MIR out of stanc3, four ways.
#
# The runtime usually embeds stanc3 -- the released library links the
# OCaml compiler in, so `stanli_model(code = ...)` hands the source
# straight to it and none of this runs.
#
# When it does not (a source build, or the Windows runtime), native hosts
# first use a compiler executable. The Windows runtime ships stanli-compile,
# which emits portable MIR through the shared OCaml pipeline, beside pristine
# stanc for one rollback cycle. Without either, the fallback is stanc3 compiled
# to JavaScript and run through V8. That is the same
# trick rstan uses to ship a Stan compiler on CRAN: js_of_ocaml turns the
# OCaml into one 3.0 MB file with no toolchain and no platform binaries,
# which is a thing CRAN can carry and a native `stanc` per platform is
# not. CI checks that this generated file comes from the exact configured
# stanc3 source revision, and the R tests compile both valid and invalid
# models through it on every supported host.
#
# Under webR there is no V8 package and no process to run a binary in,
# but the host already IS a JavaScript engine: the webr support
# package's eval_js() evaluates in the worker's global scope, and the
# same bundled stanc.js defines stanc() there once per session.
#
# An explicit stanc in STANLI_STANC still wins for compiler bisects. Otherwise
# the runtime-adjacent stanli-compile wins, followed by adjacent or PATH stanc.

stanc_js_ctx <- new.env(parent = emptyenv())

stanc_js_path <- function() {
  system.file("js", "stanc.js", package = "stanli")
}

# One V8 context per session: loading 3.0 MB of JavaScript takes a moment
# and the compiler is stateless afterwards.
stanc_js <- function() {
  if (!is.null(stanc_js_ctx$ctx)) return(stanc_js_ctx$ctx)
  if (!requireNamespace("V8", quietly = TRUE))
    stop("compiling Stan source needs either a runtime with the embedded ",
         "compiler, a packaged native compiler, a stanc3 binary ",
         "(STANLI_STANC), or the V8 package for the bundled JavaScript ",
         "compiler.", call. = FALSE)
  js <- stanc_js_path()
  if (!nzchar(js) || !file.exists(js))
    stop("the bundled stanc.js is missing from the installed package",
         call. = FALSE)
  ctx <- V8::v8()
  ctx$source(js)
  stanc_js_ctx$ctx <- ctx
  ctx
}

mir_from_js <- function(code, name = "stanli_model") {
  ctx <- stanc_js()
  ctx$assign("stanli_src", code)
  ctx$assign("stanli_name", name)
  # js_of_ocaml exports stanc() on globalThis under V8. It returns an
  # object with `result` on success and `errors` otherwise.
  out <- ctx$eval(
    "(function () {
       var f = (typeof stanc === 'function') ? stanc
             : (globalThis.stanc || (globalThis.module &&
                globalThis.module.exports && globalThis.module.exports.stanc));
       if (typeof f !== 'function') return JSON.stringify({e: 'no stanc()'});
       var r = f(stanli_name, stanli_src,
                 ['O1', 'debug-optimized-mir']);
       if (r.errors) return JSON.stringify({e: String(r.errors)});
       return JSON.stringify({r: r.result});
     })()")
  parsed <- jsonlite::fromJSON(out, simplifyVector = TRUE)
  if (!is.null(parsed$e))
    stop("stanc: ", paste(parsed$e, collapse = "\n"), call. = FALSE)
  parsed$r
}

# A native compiler, when one is packaged, configured, or on the PATH. The
# portable producer is deliberately only discovered beside the runtime: unlike
# stock stanc's legacy format it is a versioned stanli/runtime contract, so an
# unrelated stanli-compile on PATH must not silently pair with this library.
# Not under webR: there are no processes to run, and Sys.which warns about the
# missing `which` on every call there.
find_native_compiler <- function(
    sysname = Sys.info()[["sysname"]],
    runtime = stanli_runtime_path(),
    from_env = Sys.getenv("STANLI_STANC", ""),
    find_on_path = Sys.which) {
  if (identical(sysname, "Emscripten")) return(NULL)

  # Preserve the explicit legacy-compiler override used for stanc3 bisects.
  if (nzchar(from_env)) return(list(path = from_env, portable = FALSE))

  suffix <- if (identical(sysname, "Windows")) ".exe" else ""
  runtime_dir <- dirname(runtime)
  portable <- file.path(runtime_dir, paste0("stanli-compile", suffix))
  if (file.exists(portable) && !dir.exists(portable))
    return(list(path = portable, portable = TRUE))

  stanc_name <- paste0("stanc", suffix)
  stanc <- file.path(runtime_dir, stanc_name)
  if (file.exists(stanc) && !dir.exists(stanc))
    return(list(path = stanc, portable = FALSE))

  found <- find_on_path(stanc_name)
  if (nzchar(found))
    return(list(path = unname(found), portable = FALSE))
  NULL
}

# Read compiler output as explicit UTF-8 bytes. Captured system2 output is
# otherwise converted through the native Windows locale before R sees it.
read_compiler_output <- function(path) {
  size <- file.info(path)$size
  if (is.na(size) || size == 0) return("")
  value <- rawToChar(readBin(path, "raw", n = size))
  Encoding(value) <- "UTF-8"
  value
}

mir_from_binary <- function(compiler, code, portable = FALSE,
                            run_stanc = system2) {
  work <- tempfile("stanli-compile-")
  if (!dir.create(work))
    stop("could not create a temporary compiler directory", call. = FALSE)
  on.exit(unlink(work, recursive = TRUE, force = TRUE), add = TRUE)

  source <- file.path(work, "model.stan")
  con <- file(source, open = "wb")
  tryCatch(
    writeBin(charToRaw(enc2utf8(code)), con),
    finally = close(con))

  stdout_file <- file.path(work, "stdout")
  stderr_file <- file.path(work, "stderr")
  args <- c(if (!portable) c("--O1", "--debug-optimized-mir"),
            shQuote(source))
  status <- tryCatch(
    suppressWarnings(run_stanc(compiler, args, stdout = stdout_file,
                               stderr = stderr_file)),
    error = function(e) {
      stop("could not run the bundled Stan compiler: ", conditionMessage(e),
           call. = FALSE)
    })
  out <- read_compiler_output(stdout_file)
  diagnostics <- read_compiler_output(stderr_file)
  if (!identical(as.integer(status), 0L) || !nzchar(out))
    stop("stanc failed:\n",
         if (nzchar(diagnostics)) diagnostics else "stanc produced no MIR",
         call. = FALSE)
  # Warnings live on stderr. They must not be concatenated with the MIR; the
  # embedded compiler path also leaves successful frontend warnings internal.
  if (portable) out else sub("\\r?\\n$", "", out)
}

# The webR path. Source and MIR travel through the shared Emscripten
# filesystem rather than through eval_js values: the MIR of a real model
# is megabytes, a file path survives any marshalling limit, and neither
# string ever needs escaping into a JavaScript literal.
# The support package is reached by computed name, and deliberately not
# declared in Suggests: CRAN has an unrelated package also named `webr`,
# so a declaration resolves to the wrong one (its 17-dependency install
# is what broke the macOS and Windows check runners), and webR's own
# support package exists nowhere a checker could fetch it from. The
# sysname guard keeps CRAN's webr from ever being touched on a native
# machine that happens to have it installed.
webr_eval_js <- function() {
  if (!identical(Sys.info()[["sysname"]], "Emscripten")) return(NULL)
  pkg <- "webr"
  if (!requireNamespace(pkg, quietly = TRUE)) return(NULL)
  f <- get0("eval_js", envir = asNamespace(pkg))
  if (is.function(f)) f else NULL
}

mir_from_webr <- function(eval_js, code, name = "stanli_model") {
  if (is.null(stanc_js_ctx$webr_loaded)) {
    js <- stanc_js_path()
    if (!nzchar(js) || !file.exists(js))
      stop("the bundled stanc.js is missing from the installed package",
           call. = FALSE)
    eval_js(paste(readLines(js, warn = FALSE), collapse = "\n"))
    stanc_js_ctx$webr_loaded <- TRUE
  }
  src <- tempfile(fileext = ".stan")
  mirf <- tempfile(fileext = ".mir")
  on.exit(unlink(c(src, mirf)), add = TRUE)
  writeLines(code, src)
  status <- eval_js(sprintf("(() => {
    const src = Module.FS.readFile('%s', {encoding: 'utf8'});
    const r = globalThis.stanc(
      '%s', src, ['O1', 'debug-optimized-mir']);
    if (r.errors) return 'ERR: ' + String(r.errors);
    Module.FS.writeFile('%s', r.result);
    return 'ok';
  })()", src, name, mirf))
  if (!identical(status, "ok"))
    stop("stanc: ", sub("^ERR: ", "", status), call. = FALSE)
  paste(readLines(mirf, warn = FALSE), collapse = "\n")
}

stanc_mir <- function(code) {
  compiler <- find_native_compiler()
  if (!is.null(compiler))
    return(mir_from_binary(compiler$path, code,
                           portable = compiler$portable))
  ejs <- webr_eval_js()
  if (!is.null(ejs)) return(mir_from_webr(ejs, code))
  mir_from_js(code)
}
