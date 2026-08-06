// JSON data loading (CmdStan conventions): scalars, arrays, nested arrays.
#include <stanli/data.hpp>

#include <cstdio>
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

  bool threw = false;
  try {
    (void)m.at("absent");
  } catch (const std::exception& e) {
    threw = std::string(e.what()).find("absent") != std::string::npos;
  }
  check(threw, "missing name reported");

  if (failures == 0) std::printf("test_data OK\n");
  return failures == 0 ? 0 : 1;
}
