// Spike: print the lowered op sequence of a model, to check how regular
// the unrolled-loop region is (re-roll pass feasibility).
#include <stanrt/compile.hpp>
#include <stanrt/graph.hpp>
#include <stanrt/optable.hpp>

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

static const char* NAMES[] = {
    "?",         "EXP",        "ADD_N",     "BCAST_FMA", "MATVEC",
    "NORMAL",    "CAUCHY",     "STUDENT_T", "GAMMA",     "BETA",
    "POIS_LOG",  "BERN_LOGIT", "LOGNORMAL", "UNIFORM",   "DBL_EXP",
    "EXPON",     "INV_GAMMA",  "STD_NORM",  "BERN",      "POIS",
    "NEG_BIN2",  "BINOM",      "BINOM_LGT", "BERN_GLM",  "LOGIT",
    "MEAN",      "REP_VEC",    "INDEX",     "SET_INDEX", "SLICE",
    "SET_SLICE", "SLICE_STR",  "GATHER",    "CONCAT2",   "REP_MAT",
    "LSE",       "LSE2",       "LOG_MIX",   "SOFTMAX",   "SUM_VEC",
    "ADD",       "SUB",        "MUL",       "DIV",       "POW",
    "DOT",       "NEG",        "EXPV",      "LOGV",      "INV_LOGIT",
    "SQRT",      "SQUARE",     "LOG1M",     "TANHV",     "CUMSUM",
    "C_LOWER",   "C_UPPER",    "C_LU",      "C_SIMPLEX", "C_ORDERED",
    "C_POS_ORD", "DIRICHLET"};

static std::string slurp(const char* p) {
  std::ifstream f(p);
  std::ostringstream ss;
  ss << f.rdbuf();
  return ss.str();
}

int main(int argc, char** argv) {
  if (argc < 3) {
    std::fprintf(stderr, "usage: dump_ops mir.sexp data.json [max_ops]\n");
    return 2;
  }
  const int max_ops = argc > 3 ? std::atoi(argv[3]) : 200;
  stanrt::DataMap data = stanrt::DataMap::from_json(slurp(argv[2]));
  stanrt::CompiledModel cm = stanrt::compile_model(slurp(argv[1]), data);
  const stanrt::Graph& g = cm.graph;
  std::printf("slots=%zu ops=%zu result=%d\n", g.slots.size(), g.ops.size(),
              g.result_slot);
  for (size_t i = 0; i < g.ops.size() && (int)i < max_ops; ++i) {
    const stanrt::Op& op = g.ops[i];
    std::printf("%5zu %-10s v=%02x out=s%d(len%lld)", i, NAMES[op.opcode],
                op.variant, op.out, (long long)g.slots[op.out].len);
    std::printf(" in=");
    for (int k = 0; k < op.n_in; ++k) {
      std::printf("%ss%d(l%lld%s)", k ? "," : "", op.in[k],
                  (long long)g.slots[op.in[k]].len,
                  g.slots[op.in[k]].is_param ? ",P" : "");
    }
    if (op.n_idata) {
      std::printf(" idata=[");
      for (int64_t k = 0; k < op.n_idata && k < 4; ++k)
        std::printf("%s%d", k ? "," : "", op.idata[k]);
      if (op.n_idata > 4) std::printf(",...x%lld", (long long)op.n_idata);
      std::printf("]");
    }
    std::printf("\n");
  }
  return 0;
}
