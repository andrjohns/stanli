// JSON data loading (CmdStan conventions): scalars, arrays, nested arrays.
#include <stanli/data.hpp>

#ifdef DATAMAP_FROM_CONTEXT
#include <stan/io/dump.hpp>
#endif

#include <cstdio>
#ifdef DATAMAP_FROM_CONTEXT
#include <sstream>
#endif
#include <string>

static int failures = 0;
static void check(bool ok, const std::string& what) {
  if (!ok) {
    ++failures;
    std::printf("FAIL %s\n", what.c_str());
  }
}

int main() {
  using stanli::DataMap;

#ifdef DATAMAP_FROM_JSON
  DataMap d = DataMap::from_json_file("tests/fixtures/eight_schools.json");
  check(d.at("J").is_int && d.at("J").i[0] == 8, "J int 8");
  check(d.at("y").r.size() == 8 && d.at("y").r[0] == 28 && d.at("y").r[2] == -3,
        "y values");
  check(d.at("sigma").dims.size() == 1 && d.at("sigma").dims[0] == 8,
        "sigma dims");

  // Mixed int/real detection + matrices as nested arrays (row-major).
  DataMap m = DataMap::from_json(R"({
    "n": 3, "x": 2.5, "v": [1, 2, 3],
    "M": [[1.0, 2.0], [3.0, 4.0], [5.0, 6.0]],
    "yi": [0, 1, 1]
  })");
  check(m.at("n").is_int, "n is int");
  check(!m.at("x").is_int && m.at("x").r[0] == 2.5, "x real");
  check(m.at("v").is_int && m.at("v").i.size() == 3, "int array stays int");
  // Column-major: M = [[1,2],[3,4],[5,6]] stores as 1,3,5,2,4,6.
  check(m.at("M").dims.size() == 2 && m.at("M").dims[0] == 3 &&
            m.at("M").dims[1] == 2 && m.at("M").r.size() == 6 &&
            m.at("M").r[1] == 3.0 && m.at("M").r[3] == 2.0,
        "matrix column-major with dims");
  check(m.at("yi").is_int && m.at("yi").i[2] == 1, "yi int array");
#endif

#ifdef DATAMAP_FROM_CONTEXT
  // A Stan var_context has separate real and integer name lists.  The
  // constructor copies both without changing the context's column-major
  // storage order.
  DataMap from_context;
  {
    std::istringstream input(R"(
      n <- 4
      x <- 1.5
      rv <- c(0.5, -1.25, 3.75)
      iv <- c(7, -2, 0)
      R <- structure(c(1.5, 3.5, 5.5, 2.5, 4.5, 6.5), .Dim = c(3, 2))
      I <- structure(c(1, 4, 2, 5, 3, 6), .Dim = c(2, 3))
      RA <- structure(c(0.5, 1.5, 2.5, 3.5, 4.5, 5.5, 6.5, 7.5),
                      .Dim = c(2, 2, 2))
      IA <- structure(c(1, 2, 3, 4, 5, 6, 7, 8), .Dim = c(2, 2, 2))
      Z <- structure(double(0), .Dim = c(2, 0, 3))
    )");
    stan::io::dump context(input);
    from_context = DataMap(context);
  }
  check(from_context.at("n").is_int && from_context.at("n").i[0] == 4 &&
            from_context.at("n").r[0] == 4.0 &&
            from_context.at("n").dims.empty(),
        "var_context integer scalar");
  check(!from_context.at("x").is_int && from_context.at("x").r[0] == 1.5 &&
            from_context.at("x").dims.empty(),
        "var_context real scalar");
  check(!from_context.at("rv").is_int &&
            from_context.at("rv").dims == std::vector<int64_t>({3}) &&
            from_context.at("rv").r ==
                std::vector<double>({0.5, -1.25, 3.75}),
        "var_context real array");
  check(from_context.at("iv").is_int &&
            from_context.at("iv").dims == std::vector<int64_t>({3}) &&
            from_context.at("iv").i == std::vector<int>({7, -2, 0}) &&
            from_context.at("iv").r == std::vector<double>({7, -2, 0}),
        "var_context integer array");
  check(from_context.at("R").dims == std::vector<int64_t>({3, 2}) &&
            from_context.at("R").r ==
                std::vector<double>({1.5, 3.5, 5.5, 2.5, 4.5, 6.5}),
        "var_context real matrix");
  check(from_context.at("I").is_int &&
            from_context.at("I").dims == std::vector<int64_t>({2, 3}) &&
            from_context.at("I").i == std::vector<int>({1, 4, 2, 5, 3, 6}) &&
            from_context.at("I").r ==
                std::vector<double>({1, 4, 2, 5, 3, 6}),
        "var_context integer matrix");
  check(!from_context.at("RA").is_int &&
            from_context.at("RA").dims ==
                std::vector<int64_t>({2, 2, 2}) &&
            from_context.at("RA").r ==
                std::vector<double>({0.5, 1.5, 2.5, 3.5, 4.5, 5.5, 6.5,
                                     7.5}),
        "var_context nested real array");
  check(from_context.at("IA").is_int &&
            from_context.at("IA").dims ==
                std::vector<int64_t>({2, 2, 2}) &&
            from_context.at("IA").i ==
                std::vector<int>({1, 2, 3, 4, 5, 6, 7, 8}) &&
            from_context.at("IA").r ==
                std::vector<double>({1, 2, 3, 4, 5, 6, 7, 8}),
        "var_context nested integer array");
  check(from_context.at("Z").dims == std::vector<int64_t>({2, 0, 3}) &&
            from_context.at("Z").i.empty() && from_context.at("Z").r.empty(),
        "var_context zero-sized nested array");
#endif

  bool threw = false;
  try {
    (void)DataMap().at("absent");
  } catch (const std::exception& e) {
    threw = std::string(e.what()).find("absent") != std::string::npos;
  }
  check(threw, "missing name reported");

  if (failures == 0) std::printf("test_data OK\n");
  return failures == 0 ? 0 : 1;
}
