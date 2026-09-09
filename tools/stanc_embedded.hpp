#pragma once

#include <array>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>

#ifdef STANLI_EMBED_STANC
extern "C" char* stanli_stanc_tmir(const char* stan_code);
extern "C" void stanli_stanc_free(char* p);

namespace stanli::tooling {

inline std::string embedded_stanc(const std::string& model) {
  std::string src;
  {
    std::unique_ptr<FILE, int (*)(FILE*)> f(std::fopen(model.c_str(), "rb"),
                                            std::fclose);
    if (!f) throw std::runtime_error("cannot read " + model);
    std::array<char, 1 << 16> buf;
    size_t n;
    while ((n = fread(buf.data(), 1, buf.size(), f.get())) > 0)
      src.append(buf.data(), n);
  }
  char* res = stanli_stanc_tmir(src.c_str());
  const std::string out(res ? res : "ERRstanc returned nothing");
  if (res) stanli_stanc_free(res);
  if (out.compare(0, 3, "ERR") == 0)
    throw std::runtime_error("stanc: " + out.substr(3));
  return out.substr(2);
}

}  // namespace stanli::tooling
#endif
