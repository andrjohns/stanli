#include <stanli/function.hpp>
#include <stanli/capi.h>

#include <cstdio>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

int failures = 0;

void check(bool condition, const char* message) {
  if (!condition) {
    ++failures;
    std::printf("FAIL %s\n", message);
  }
}

template <typename F>
void throws_with(F&& f, const std::string& needle, const char* message) {
  try {
    f();
    check(false, message);
  } catch (const std::exception& e) {
    check(std::string(e.what()).find(needle) != std::string::npos, message);
  }
}

const char* source = R"stan(
functions {
  vector affine(vector x, real a, real b) {
    return a * x + b;
  }
  int plus_one(int x) {
    return x + 1;
  }
  matrix scale_matrix(matrix x, real a) {
    return a * x;
  }
  real overloaded(real x) {
    return x + 0.5;
  }
  real overloaded(vector x) {
    return sum(x);
  }
  real numeric(int x) {
    return x + 10;
  }
  real numeric(real x) {
    return x + 20;
  }
}
model {}
)stan";

std::string slurp(const char* path) {
  std::ifstream input(path);
  std::ostringstream text;
  text << input.rdbuf();
  return text.str();
}

}  // namespace

int main() {
  using stanli::DataMap;
  using stanli::Function;

  try {
    // The MIR constructor works in every runtime build, including developer
    // builds which deliberately omit the embedded source compiler.
    Function cached = Function::from_mir(
        slurp("tests/fixtures/early_return.tmir.sexp"), "choose");
    DataMap cached_args;
    cached_args.set_real("x", 4.0);
    cached_args.set_int("first", 1);
    check(cached(cached_args).r == std::vector<double>({8.0}),
          "cached MIR function call");

    if (!stanli_has_embedded_stanc()) {
      throws_with([&] { Function unavailable(source, "affine"); },
                  "does not embed stanc3",
                  "source constructor reports unavailable compiler");
      if (failures == 0) std::printf("OK function\n");
      return failures == 0 ? 0 : 1;
    }

    Function affine(source, "affine");
    DataMap args;
    args.set_real_array("x", {1.0, 2.0, 4.0});
    args.set_real("a", 2.5);
    args.set_real("b", -1.0);
    const DataMap::Entry result = affine(args);
    check(!result.is_int, "vector result is real");
    check(result.dims == std::vector<int64_t>{3},
          "vector result keeps dimensions");
    check(result.r == std::vector<double>({1.5, 4.0, 9.0}),
          "vector result values");

    Function plus_one(source, "plus_one");
    DataMap ints;
    ints.set_int("x", 41);
    const DataMap::Entry integer = plus_one(ints);
    check(integer.is_int && integer.i == std::vector<int>({42}) &&
              integer.r == std::vector<double>({42.0}) && integer.dims.empty(),
          "integer result keeps both representations");

    Function scale(source, "scale_matrix");
    DataMap matrix;
    // Matrix storage is column-major: [1 3; 2 4].
    matrix.set_real_array("x", {1.0, 2.0, 3.0, 4.0}, {2, 2});
    matrix.set_real("a", 3.0);
    const DataMap::Entry scaled = scale(matrix);
    check(scaled.dims == std::vector<int64_t>({2, 2}),
          "matrix result keeps dimensions");
    check(scaled.r == std::vector<double>({3.0, 6.0, 9.0, 12.0}),
          "matrix result keeps column-major values");

    Function overloaded(source, "overloaded");
    DataMap scalar;
    scalar.set_real("x", 2.0);
    check(overloaded(scalar).r == std::vector<double>({2.5}),
          "overload selected by scalar rank");
    DataMap vector;
    vector.set_real_array("x", {1.0, 2.0, 3.0});
    check(overloaded(vector).r == std::vector<double>({6.0}),
          "overload selected by vector rank");

    Function numeric(source, "numeric");
    DataMap promoted;
    promoted.set_int("x", 2);
    check(numeric(promoted).r == std::vector<double>({12.0}),
          "integer overload wins over real promotion");

    throws_with(
        [&] {
          DataMap missing;
          missing.set_real("a", 1.0);
          missing.set_real("b", 2.0);
          (void)affine(missing);
        },
        "variable not provided: x", "missing named argument reports its name");

    throws_with(
        [&] {
          DataMap wrong;
          wrong.set_real("x", 1.0);
          wrong.set_real("a", 1.0);
          wrong.set_real("b", 2.0);
          (void)affine(wrong);
        },
        "rank 0, expected 1", "argument rank mismatch is rejected");

    throws_with([&] { Function absent(source, "absent"); },
                "Stan function not found", "missing function is rejected");

    const DataMap::Entry one_shot =
        stanli::evaluate_function(source, "plus_one", ints);
    check(one_shot.i == std::vector<int>({42}),
          "one-shot evaluate_function entry point");
  } catch (const std::exception& e) {
    std::printf("FAIL unexpected exception: %s\n", e.what());
    ++failures;
  }

  if (failures == 0) std::printf("OK function\n");
  return failures == 0 ? 0 : 1;
}
