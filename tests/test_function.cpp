#include <stanli/function.hpp>
#include <stanli/capi.h>

#include <cstdio>
#include <fstream>
#include <limits>
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
  real real_identity(real x) {
    return x;
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
  real direction(vector x) {
    return sum(x);
  }
  real direction(row_vector x) {
    return -sum(x);
  }
  real descend(real x, int remaining) {
    if (remaining == 0) return x;
    return descend(x + 1, remaining - 1);
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

void typed_boundary(const std::string& mir) {
  char err[8192] = {};
  auto* f =
      stanli_function_new_from_mir(mir.c_str(), "choose", err, sizeof(err));
  check(f != nullptr, "typed boundary constructs from MIR");
  if (!f) return;
  double x = 4.0;
  int first = 1;
  stanli_function_argument args[] = {
      {"x", 0, &x, nullptr, 1, nullptr, 0},
      {"first", 1, nullptr, &first, 1, nullptr, 0}};
  double result = 0;
  const auto writer = [](void* context, int is_int, const double* reals,
                         size_t real_size, const int*, size_t, const int64_t*,
                         size_t dim_size) -> int {
    if (is_int || real_size != 1 || dim_size != 0) return 1;
    *static_cast<double*>(context) = reals[0];
    return 0;
  };
  const auto call = [&] {
    return stanli_function_call_values(f, args, 2, writer, &result, err,
                                       sizeof(err));
  };
  check(call() == 0 && result == 8.0,
        "typed boundary real and integer scalars");
  args[0].size = 0;
  check(call() != 0, "typed boundary rejects invalid scalar length");
  args[0].size = 1;
  args[1].ints = nullptr;
  check(call() != 0, "typed boundary rejects null nonempty integer buffer");
  args[1].ints = &first;
  args[0].is_int = 2;
  check(call() != 0, "typed boundary rejects invalid type discriminator");
  args[0].is_int = 0;
  args[0].name = "first";
  check(call() != 0, "typed boundary rejects duplicate names");
  args[0].name = "x";
  args[0].dim_size = 1;
  check(call() != 0, "typed boundary rejects null dimension buffer");
  int64_t dims[] = {-1, 2};
  args[0].dims = dims;
  check(call() != 0, "typed boundary rejects negative dimensions");
  dims[0] = std::numeric_limits<int64_t>::max();
  args[0].dim_size = 2;
  check(call() != 0, "typed boundary rejects overflowing dimensions");
  args[0].dims = nullptr;
  args[0].dim_size = 0;
  check(call() == 0 && result == 8.0, "typed boundary recovers after errors");
  check(stanli_function_call_values(f, nullptr, 2, writer, &result, err,
                                    sizeof(err)) != 0,
        "typed boundary rejects null argument table");
  stanli_function_free(f);
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
    typed_boundary(slurp("tests/fixtures/early_return.tmir.sexp"));

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

    Function real_identity(source, "real_identity");
    const DataMap::Entry promoted_real = real_identity(ints);
    check(!promoted_real.is_int && promoted_real.i.empty() &&
              promoted_real.r == std::vector<double>({41.0}),
          "integer argument is promoted to the real formal");

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
    for (int i = 0; i < 3; ++i) {
      check(numeric(scalar).r == std::vector<double>({22.0}),
            "cached candidates select a real overload after an integer");
      check(numeric(promoted).r == std::vector<double>({12.0}),
            "cached candidates select an integer overload after a real");
      throws_with([&] { (void)numeric(vector); }, "no overload",
                  "cached candidates still reject invalid ranks");
      check(overloaded(vector).r == std::vector<double>({6.0}) &&
                overloaded(scalar).r == std::vector<double>({2.5}),
            "cached candidates select by the current rank");
    }
    Function resolved(source, "numeric(real)");
    check(resolved(promoted).r == std::vector<double>({22.0}),
          "resolved signature preserves promotion despite integer overload");
    Function direction(source, "direction");
    throws_with([&] { (void)direction(vector); }, "ambiguously match",
                "cached candidates preserve vector/row-vector ambiguity");
    Function resolved_direction(source, "direction(vector)");
    check(resolved_direction(vector).r == std::vector<double>({6.0}),
          "resolved signature disambiguates identical host ranks");

    // Recursive calls cannot be inlined away: they exercise the cached full
    // definition table, not just the top-level candidate list.
    Function descend(source, "descend");
    DataMap recursive;
    recursive.set_real("x", 1.5);
    recursive.set_int("remaining", 3);
    check(descend(recursive).r == std::vector<double>({4.5}),
          "cached table supports nested user-function calls");
    recursive.set_int("remaining", 70);
    throws_with([&] { (void)descend(recursive); }, "recursion too deep",
                "cached table preserves recursion guard");
    recursive.set_int("remaining", 2);
    check(descend(recursive).r == std::vector<double>({3.5}),
          "cached table recovers after an interpreter failure");

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
