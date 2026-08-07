// setenv/unsetenv for the tests, portable to Windows: the CRT has
// _putenv_s instead, and passing it an empty value removes the variable,
// which is exactly unsetenv.
#ifndef STANLI_TESTS_ENV_HELPERS_HPP
#define STANLI_TESTS_ENV_HELPERS_HPP

#include <cstdlib>

#ifdef _WIN32
inline int test_setenv(const char* k, const char* v, int = 1) {
  return _putenv_s(k, v);
}
inline int test_unsetenv(const char* k) { return _putenv_s(k, ""); }
#else
inline int test_setenv(const char* k, const char* v, int overwrite = 1) {
  return setenv(k, v, overwrite);
}
inline int test_unsetenv(const char* k) { return unsetenv(k); }
#endif

#endif
