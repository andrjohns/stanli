// C bridge to the embedded stanc3 (OCaml, linked in via
// -output-complete-obj). Compiled into libstanrt only when the embed object
// is available (STANRT_EMBED_STANC).
//
// OCaml runtime notes: caml_startup runs once; callbacks must come from a
// thread known to the OCaml runtime. v1 policy: all stanc calls happen on
// the thread that first called it (Python's ctypes calls satisfy this).
#include <caml/alloc.h>
#include <caml/callback.h>
#include <caml/mlvalues.h>

#include <cstdlib>
#include <cstring>
#include <mutex>

extern "C" {

// "OK<sexp>" or "ERR<message>"; caller frees with stanrt_stanc_free.
char* stanrt_stanc_tmir(const char* stan_code) {
  static std::once_flag once;
  std::call_once(once, [] {
    static char arg0[] = "stanrt";
    static char* argv[] = {arg0, nullptr};
    caml_startup(argv);
  });
  static const value* fn = nullptr;
  if (fn == nullptr) fn = caml_named_value("stanc_compile_tmir");
  if (fn == nullptr) {
    return strdup("ERRembedded stanc entry point not registered");
  }
  value res = caml_callback(*fn, caml_copy_string(stan_code));
  return strdup(String_val(res));
}

void stanrt_stanc_free(char* p) { std::free(p); }

}  // extern "C"
