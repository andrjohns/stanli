#include <stanrt/data.hpp>

#include "../third_party/nlohmann_json.hpp"

#include <fstream>
#include <functional>
#include <sstream>

namespace stanrt {

using nlohmann::json;

static DataMap::Entry entry_from_json(const std::string& name, const json& v) {
  DataMap::Entry e;
  if (v.is_number_integer()) {
    e.is_int = true;
    e.i = {v.get<int>()};
    return e;
  }
  if (v.is_number()) {
    e.r = {v.get<double>()};
    return e;
  }
  if (v.is_array()) {
    if (v.empty()) {
      e.dims = {0};
      return e;
    }
    if (v[0].is_array()) {
      // Nested array -> matrix (row-major), or deeper arrays flattened with
      // dims outer-to-inner.
      std::vector<int64_t> dims;
      const json* cur = &v;
      while (cur->is_array() && !cur->empty()) {
        dims.push_back(static_cast<int64_t>(cur->size()));
        cur = &(*cur)[0];
      }
      e.dims = dims;
      std::function<void(const json&)> flat = [&](const json& node) {
        if (node.is_array()) {
          for (const auto& k : node) flat(k);
        } else {
          e.r.push_back(node.get<double>());
        }
      };
      flat(v);
      return e;
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
      for (const auto& k : v) e.r.push_back(k.get<double>());
    }
    return e;
  }
  throw std::runtime_error("data: unsupported JSON value for " + name);
}

DataMap DataMap::from_json(const std::string& text) {
  json root = json::parse(text);
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

}  // namespace stanrt
