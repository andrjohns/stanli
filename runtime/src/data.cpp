#include <stanli/data.hpp>

#include "../third_party/nlohmann_json.hpp"

#include <cstring>
#include <fstream>
#include <functional>
#include <limits>
#include <sstream>

namespace stanli {

using nlohmann::json;

// The spellings CmdStan's JSON reader accepts: rapidjson's kParseNanAndInfFlag
// takes them bare, json_data_handler::string() takes them quoted. Longest
// prefix first, so the rewriter below does not stop at "Inf" inside
// "Infinity".
static const struct {
  const char* text;
  double value;
} kNonFinite[] = {
    {"-Infinity", -std::numeric_limits<double>::infinity()},
    {"-Inf", -std::numeric_limits<double>::infinity()},
    {"Infinity", std::numeric_limits<double>::infinity()},
    {"Inf", std::numeric_limits<double>::infinity()},
    {"NaN", std::numeric_limits<double>::quiet_NaN()},
};

static bool nonfinite_value(const std::string& s, double* out) {
  for (const auto& t : kNonFinite)
    if (s == t.text) {
      *out = t.value;
      return true;
    }
  return false;
}

// nlohmann's lexer has no option for the bare tokens, so quote them and let
// the string case below decode both spellings the same way.
static std::string quote_nonfinite(const std::string& text) {
  std::string out;
  out.reserve(text.size());
  bool in_string = false;
  size_t i = 0;
  while (i < text.size()) {
    const char c = text[i];
    if (in_string) {
      out += c;
      if (c == '\\' && i + 1 < text.size())
        out += text[++i];
      else if (c == '"')
        in_string = false;
      ++i;
      continue;
    }
    if (c == '"') {
      in_string = true;
      out += c;
      ++i;
      continue;
    }
    const char* match = nullptr;
    for (const auto& t : kNonFinite)
      if (text.compare(i, std::strlen(t.text), t.text) == 0) {
        match = t.text;
        break;
      }
    if (!match) {
      out += c;
      ++i;
      continue;
    }
    out += '"';
    out += match;
    out += '"';
    i += std::strlen(match);
  }
  return out;
}

static double element_value(const json& v, const std::string& name) {
  if (v.is_number()) return v.get<double>();
  double d;
  if (v.is_string() && nonfinite_value(v.get<std::string>(), &d)) return d;
  throw std::runtime_error("data: unsupported JSON value for " + name);
}

static DataMap::Entry entry_from_json(const std::string& name, const json& v) {
  DataMap::Entry e;
  if (v.is_number_integer()) {
    e.is_int = true;
    e.i = {v.get<int>()};
    e.r = {v.get<double>()};  // int scalars are usable wherever reals are
    return e;
  }
  if (v.is_array()) {
    if (v.empty()) {
      // An empty array is vacuously all-int (R's integer(0) arrives as []),
      // and its empty real side satisfies a real declaration just as well,
      // so claiming int here never misleads: empty satisfies both types.
      e.is_int = true;
      e.dims = {0};
      return e;
    }
    if (v[0].is_array()) {
      // Nested array -> matrix (row-major), or deeper arrays flattened with
      // dims outer-to-inner.
      std::vector<int64_t> dims;
      const json* cur = &v;
      // Include the first zero extent: [[], []] is array[2] vector[0], not
      // a one-dimensional value whose elements should be parsed as numbers.
      while (cur->is_array()) {
        dims.push_back(static_cast<int64_t>(cur->size()));
        if (cur->empty()) break;
        cur = &(*cur)[0];
      }
      e.dims = dims;
      if (dims.size() == 2) {
        // Column-major, the Stan/Eigen convention (and what stanc's data
        // reconstruction loops assume for the flat read buffer).
        const int64_t R = dims[0], C = dims[1];
        e.r.resize(R * C);
        bool all_int = true;
        for (int64_t i = 0; i < R; ++i)
          for (int64_t j = 0; j < C; ++j) {
            e.r[j * R + i] = element_value(v[i][j], name);
            if (!v[i][j].is_number_integer()) all_int = false;
          }
        if (all_int) {
          e.is_int = true;
          e.i.resize(R * C);
          for (int64_t i = 0; i < R; ++i)
            for (int64_t j = 0; j < C; ++j) e.i[j * R + i] = v[i][j].get<int>();
        }
        return e;
      }
      // N-D (>2): column-major like everything else (first index fastest).
      {
        const std::vector<int64_t>& D = e.dims;
        std::vector<int64_t> stride(D.size());
        int64_t total = 1;
        for (size_t d = 0; d < D.size(); ++d) {
          stride[d] = total;
          total *= D[d];
        }
        e.r.assign(total, 0.0);
        bool all_int = true;
        std::vector<int64_t> ix(D.size(), 0);
        std::function<void(const json&, size_t)> walk = [&](const json& node,
                                                            size_t depth) {
          if (depth == D.size()) {
            int64_t flatpos = 0;
            for (size_t d = 0; d < D.size(); ++d) flatpos += ix[d] * stride[d];
            e.r[flatpos] = element_value(node, name);
            if (!node.is_number_integer()) all_int = false;
            return;
          }
          for (size_t k = 0; k < node.size(); ++k) {
            ix[depth] = (int64_t)k;
            walk(node[k], depth + 1);
          }
        };
        walk(v, 0);
        if (all_int) {
          e.is_int = true;
          e.i.resize(total);
          for (int64_t k = 0; k < total; ++k) e.i[k] = (int)e.r[k];
        }
        return e;
      }
    }
    bool all_int = true;
    for (const auto& k : v)
      if (!k.is_number_integer()) all_int = false;
    e.dims = {static_cast<int64_t>(v.size())};
    if (all_int) {
      e.is_int = true;
      for (const auto& k : v) e.i.push_back(k.get<int>());
      // Int arrays also usable as reals.
      for (const auto& k : v) e.r.push_back(k.get<double>());
    } else {
      for (const auto& k : v) e.r.push_back(element_value(k, name));
    }
    return e;
  }
  e.r = {element_value(v, name)};
  return e;
}

DataMap DataMap::from_json(const std::string& text) {
  const bool bare = text.find("Inf") != std::string::npos ||
                    text.find("NaN") != std::string::npos;
  json root = json::parse(bare ? quote_nonfinite(text) : text);
  if (!root.is_object())
    throw std::runtime_error("data: top-level JSON must be an object");
  DataMap d;
  for (auto it = root.begin(); it != root.end(); ++it)
    d.m_[it.key()] = entry_from_json(it.key(), it.value());
  return d;
}

DataMap DataMap::from_json_file(const std::string& path) {
  std::ifstream f(path);
  if (!f) throw std::runtime_error("data: cannot open " + path);
  std::ostringstream ss;
  ss << f.rdbuf();
  return from_json(ss.str());
}

}  // namespace stanli
