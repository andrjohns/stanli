#include "../tools/stanc_process.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;

int main(int argc, char** argv) {
  // A copied instance of this test is the fake compiler used by the parent.
  if (argc > 1) {
    if (argc != 4 || std::string(argv[1]) != "--O1" ||
        std::string(argv[2]) != "--debug-optimized-mir")
      return 2;
    std::cout << "MIR from " << argv[3] << '\n';
    return 0;
  }

  const auto nonce =
      std::chrono::high_resolution_clock::now().time_since_epoch().count();
  const fs::path root = fs::temp_directory_path() /
                        ("stanli stanc ' process " + std::to_string(nonce));
  try {
    fs::create_directories(root);
#ifdef _WIN32
    const fs::path compiler = root / "fake stanc ' compiler.exe";
#else
    const fs::path compiler = root / "fake stanc ' compiler";
#endif
    fs::copy_file(fs::absolute(argv[0]), compiler);
    fs::permissions(
        compiler,
        fs::perms::owner_exec | fs::perms::group_exec | fs::perms::others_exec,
        fs::perm_options::add);
    const fs::path model = root / "model with ' quote.stan";
    std::ofstream(model)
        << "parameters { real y; } model { y ~ normal(0, 1); }\n";

    const std::string output =
        stanli::tooling::run_stanc_process(compiler.string(), model.string());
    const std::string expected = "MIR from " + model.string() + "\n";
    if (output != expected)
      throw std::runtime_error("unexpected compiler output: " + output);
    fs::remove_all(root);
    return 0;
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    std::error_code ignored;
    fs::remove_all(root, ignored);
    return 1;
  }
}
