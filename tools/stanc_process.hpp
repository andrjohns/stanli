#pragma once

#include <string>

namespace stanli::tooling {

// Run stock stanc with the flags used by the command-line tools and return
// its MIR stdout. Arguments are passed directly to the child process: model
// and compiler paths are never interpreted by a shell.
std::string run_stanc_process(const std::string& stanc,
                              const std::string& model);

}  // namespace stanli::tooling
