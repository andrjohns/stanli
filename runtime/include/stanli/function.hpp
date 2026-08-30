// Direct, value-only invocation of a Stan user-defined function.
//
// This is a C++ convenience surface over exported stanli_* symbols.  It uses
// DataMap for named arguments and DataMap::Entry for the result, preserving
// integer identity, logical dimensions, and Stan's column-major storage.
#ifndef STANLI_FUNCTION_HPP
#define STANLI_FUNCTION_HPP

#include <stanli/data.hpp>

#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>

struct stanli_function;

extern "C" {

stanli_function* stanli_function_new_from_stan(const char* stan_code,
                                               const char* function_name,
                                               char* err, size_t err_len);
stanli_function* stanli_function_new_from_mir(const char* mir_text,
                                              const char* function_name,
                                              char* err, size_t err_len);
void stanli_function_free(stanli_function* function);
int stanli_function_call(const stanli_function* function,
                         const stanli::DataMap* arguments,
                         stanli::DataMap::Entry* result, char* err,
                         size_t err_len);

}  // extern "C"

namespace stanli {

class Function {
 public:
  Function(const std::string& stan_code, const std::string& function_name)
      : handle_(construct(stan_code, function_name, false)) {}

  static Function from_mir(const std::string& mir_text,
                           const std::string& function_name) {
    return Function(construct(mir_text, function_name, true));
  }

  ~Function() { stanli_function_free(handle_); }

  Function(const Function&) = delete;
  Function& operator=(const Function&) = delete;

  Function(Function&& other) noexcept
      : handle_(std::exchange(other.handle_, nullptr)) {}
  Function& operator=(Function&& other) noexcept {
    if (this != &other) {
      stanli_function_free(handle_);
      handle_ = std::exchange(other.handle_, nullptr);
    }
    return *this;
  }

  DataMap::Entry operator()(const DataMap& arguments) const {
    char err[8192] = {};
    DataMap::Entry result;
    if (stanli_function_call(handle_, &arguments, &result, err, sizeof(err)))
      throw std::runtime_error(err[0] ? err
                                      : "Stan function evaluation failed");
    return result;
  }

 private:
  explicit Function(stanli_function* handle) : handle_(handle) {}

  static stanli_function* construct(const std::string& text,
                                    const std::string& name, bool mir) {
    char err[8192] = {};
    stanli_function* result =
        mir ? stanli_function_new_from_mir(text.c_str(), name.c_str(), err,
                                           sizeof(err))
            : stanli_function_new_from_stan(text.c_str(), name.c_str(), err,
                                            sizeof(err));
    if (!result)
      throw std::runtime_error(err[0] ? err
                                      : "Stan function compilation failed");
    return result;
  }

  stanli_function* handle_ = nullptr;
};

inline DataMap::Entry evaluate_function(const std::string& stan_code,
                                        const std::string& function_name,
                                        const DataMap& arguments) {
  return Function(stan_code, function_name)(arguments);
}

}  // namespace stanli

#endif
