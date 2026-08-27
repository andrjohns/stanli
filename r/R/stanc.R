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
# to JavaScript and run through V8. js_of_ocaml turns the OCaml into one
# 3.0 MB file with no toolchain or platform-specific binary, and the same
# artifact can execute inside webR. CI checks that this generated file comes
# from the exact configured stanc3 source revision, and the R tests compile
# both valid and invalid models through it on every supported host.
#
# Under webR there is no V8 package and no process to run a binary in,
# but the host already IS a JavaScript engine: the webr support
# package's eval_js() evaluates in the worker's global scope, and the
# same bundled stanc.js defines stanli_compile() and its stanc-compatible export
# there once per session. Both native V8 and webR select the shared portable
# producer by export presence.
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
  ctx$assign("stanli_src", enc2utf8(code))
  ctx$assign("stanli_name", name)
  # Prefer the portable producer by export presence. Once selected, its
  # diagnostics are final: a bad model must not be compiled again through the
  # legacy producer and accidentally turn a real error into different output.
  out <- ctx$eval(
    "(function () {
       function exported(name) {
         if (Object.prototype.hasOwnProperty.call(globalThis, name))
           return {present: true, value: globalThis[name]};
         var common = globalThis.module && globalThis.module.exports;
         if (common && Object.prototype.hasOwnProperty.call(common, name))
           return {present: true, value: common[name]};
         return {present: false, value: null};
       }
       var portable_export = exported('stanli_compile');
       var classic_export = exported('stanc');
       var compiler = portable_export.present ? 'stanli_compile' : 'stanc';
       var selected = portable_export.present ? portable_export
                                              : classic_export;
       if (!selected.present)
         return JSON.stringify({e: 'no stanli_compile() or stanc() export',
                                c: 'JavaScript compiler'});
       if (typeof selected.value !== 'function')
         return JSON.stringify({e: 'export is not a function', c: compiler});
       try {
         var r = portable_export.present
           ? selected.value(stanli_name, stanli_src)
           : selected.value(stanli_name, stanli_src,
                            ['O1', 'debug-optimized-mir']);
         if (!r || typeof r !== 'object')
           return JSON.stringify({e: 'compiler returned no result object',
                                  c: compiler});
         if (r.errors)
           return JSON.stringify({e: String(r.errors), c: compiler});
         if (typeof r.result === 'undefined')
           return JSON.stringify({e: 'compiler returned no MIR', c: compiler});
         return JSON.stringify({r: String(r.result), c: compiler});
       } catch (e) {
         return JSON.stringify({e: String(e), c: compiler});
       }
     })()")
  parsed <- jsonlite::fromJSON(out, simplifyVector = TRUE)
  if (!is.null(parsed$e))
    stop(parsed$c, ": ", paste(parsed$e, collapse = "\n"), call. = FALSE)
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

write_utf8_file <- function(path, value) {
  con <- file(path, open = "wb")
  tryCatch(
    writeBin(charToRaw(enc2utf8(value)), con),
    finally = close(con))
}

js_string_literal <- function(value) {
  encodeString(enc2utf8(value), quote = "'")
}

mir_from_binary <- function(compiler, code, portable = FALSE,
                            run_stanc = system2) {
  work <- tempfile("stanli-compile-")
  if (!dir.create(work))
    stop("could not create a temporary compiler directory", call. = FALSE)
  on.exit(unlink(work, recursive = TRUE, force = TRUE), add = TRUE)

  source <- file.path(work, "model.stan")
  write_utf8_file(source, code)

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
# The support package is reached by computed name and deliberately not declared
# in Suggests: the ordinary R package also named `webr` is unrelated (its
# 17-dependency install once broke the macOS and Windows check runners), while
# webR's host support package is supplied by webR itself. The sysname guard
# keeps the unrelated package from being touched on a native machine.
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
  write_utf8_file(src, code)
  src_js <- js_string_literal(src)
  name_js <- js_string_literal(name)
  mirf_js <- js_string_literal(mirf)
  status <- eval_js(sprintf("(() => {
    const src = Module.FS.readFile(%s, {encoding: 'utf8'});
    function exported(name) {
      if (Object.prototype.hasOwnProperty.call(globalThis, name))
        return {present: true, value: globalThis[name]};
      const common = globalThis.module && globalThis.module.exports;
      if (common && Object.prototype.hasOwnProperty.call(common, name))
        return {present: true, value: common[name]};
      return {present: false, value: null};
    }
    const portableExport = exported('stanli_compile');
    const classicExport = exported('stanc');
    const compiler = portableExport.present ? 'stanli_compile' : 'stanc';
    const selected = portableExport.present ? portableExport : classicExport;
    if (!selected.present)
      return 'ERR:JavaScript compiler: no stanli_compile() or stanc() export';
    if (typeof selected.value !== 'function')
      return 'ERR:' + compiler + ': export is not a function';
    try {
      const r = portableExport.present
        ? selected.value(%s, src)
        : selected.value(%s, src, ['O1', 'debug-optimized-mir']);
      if (!r || typeof r !== 'object')
        return 'ERR:' + compiler + ': compiler returned no result object';
      if (r.errors) return 'ERR:' + compiler + ': ' + String(r.errors);
      if (typeof r.result === 'undefined')
        return 'ERR:' + compiler + ': compiler returned no MIR';
      Module.FS.writeFile(%s, String(r.result));
      return 'ok';
    } catch (e) {
      return 'ERR:' + compiler + ': ' + String(e);
    }
  })()", src_js, name_js, name_js, mirf_js))
  if (!identical(status, "ok"))
    stop(sub("^ERR:", "", status), call. = FALSE)
  read_compiler_output(mirf)
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
