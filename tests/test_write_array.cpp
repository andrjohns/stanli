// CSV column naming and write_array structure.
//
// The column names are a contract with every downstream reader, and two of
// the rules were real bugs found only by diffing headers against CmdStan
// (harnesses/wa_header_check.py, which needs a CmdStan build and cannot run in
// CI): a container is indexed even at length one (`vector[1] v` is v.1, not
// v), and a matrix carries two indices in column-major order (M.1.1, M.2.1,
// M.1.2). This test pins those rules in-repo, at two levels:
//
//   * csv_names on hand-built ParamViews, one per naming rule, and
//   * the whole pipeline on tests/fixtures/wanames.stan, whose expected
//     header was taken verbatim from a CmdStan run of the same model.
#include <stanli/compile.hpp>

#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace {

int failures = 0;

void expect_eq(const std::string& what, const std::string& got,
               const std::string& want) {
  if (got != want) {
    ++failures;
    std::printf("FAIL %s\n  got  %s\n  want %s\n", what.c_str(), got.c_str(),
                want.c_str());
  }
}

std::string slurp(const std::string& path) {
  std::ifstream f(path);
  std::ostringstream ss;
  ss << f.rdbuf();
  return ss.str();
}

std::string joined(const std::vector<stanli::CompiledModel::ParamView>& cols) {
  std::string h;
  for (const auto& n : stanli::CompiledModel::csv_names(cols)) {
    if (!h.empty()) h += ',';
    h += n;
  }
  return h;
}

void test_naming_rules() {
  using PV = stanli::CompiledModel::ParamView;
  using N = PV::Naming;

  expect_eq("scalar is bare", joined({PV{"s", 0, 1, N::Scalar, 0}}), "s");
  // The length-one container: the bug was emitting "v".
  expect_eq("vector[1] is v.1", joined({PV{"v", 0, 1, N::Container, 0}}),
            "v.1");
  expect_eq("vector[3]", joined({PV{"v", 0, 3, N::Container, 0}}),
            "v.1,v.2,v.3");
  // The matrix: the bug was emitting M.1 .. M.6.
  expect_eq("matrix[2,3] col-major", joined({PV{"M", 0, 6, N::Matrix, 2}}),
            "M.1.1,M.2.1,M.1.2,M.2.2,M.1.3,M.2.3");
  // Auto (the log_prob views): bare at length one, indexed above.
  expect_eq("auto scalar", joined({PV{"a", 0, 1, N::Auto, 0}}), "a");
  expect_eq("auto vector", joined({PV{"a", 0, 2, N::Auto, 0}}), "a.1,a.2");
}

void test_wanames_pipeline() {
  using namespace stanli;
  DataMap data;  // the model takes no data
  CompiledModel cm =
      compile_model(slurp("tests/fixtures/wanames.tmir.sexp"), data);
  if (!cm.write_array || cm.write_array->columns.empty()) {
    ++failures;
    std::printf("FAIL wanames: no write_array\n");
    return;
  }
  expect_eq("wanames truncated", cm.write_array->truncated, "");
  // Taken verbatim from CmdStan's CSV for this model (diagnostic columns
  // dropped). Note gq's order: Stan flattens an array of vectors with the
  // FIRST index varying fastest, like everything else it serializes.
  expect_eq("wanames header (CmdStan-verbatim)",
            joined(cm.write_array->columns),
            "s,v.1,M.1.1,M.2.1,M.1.2,M.2.2,M.1.3,M.2.3,"
            "gq.1.1,gq.2.1,gq.1.2,gq.2.2");

  // And the write_array graph computes the right VALUES, not just names:
  // gq[i][j] = s + 10*i + j is exact arithmetic on the draw.
  Executor wex(std::move(cm.write_array->graph));
  cm.write_array->bind(wex);
  const double s = 0.375;
  wex.params_data()[0] = s;  // s is the first unconstrained parameter
  wex.run_forward_only();
  int64_t at = 0;
  double gq[4] = {0, 0, 0, 0};
  const auto names = CompiledModel::csv_names(cm.write_array->columns);
  for (const auto& c : cm.write_array->columns) {
    const double* p = wex.value_ptr(c.slot);
    for (int64_t k = 0; k < c.len; ++k, ++at)
      if (names[(size_t)at].rfind("gq.", 0) == 0) {
        const int i = names[(size_t)at][3] - '0';
        const int j = names[(size_t)at][5] - '0';
        gq[(i - 1) * 2 + (j - 1)] = p[k];
      }
  }
  for (int i = 1; i <= 2; ++i)
    for (int j = 1; j <= 2; ++j) {
      const double want = s + 10.0 * i + j;
      const double got = gq[(i - 1) * 2 + (j - 1)];
      if (got != want) {
        ++failures;
        std::printf("FAIL wanames gq.%d.%d: got %.17g want %.17g\n", i, j,
                    got, want);
      }
    }
}

}  // namespace

int main() {
  test_naming_rules();
  test_wanames_pipeline();
  if (failures == 0) std::printf("test_write_array OK\n");
  return failures == 0 ? 0 : 1;
}
