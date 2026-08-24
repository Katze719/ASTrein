#include "functions/strip_gcc_module_arguments.hpp"

#include "llvm/ADT/StringRef.h"

namespace astrein {

clang::tooling::ArgumentsAdjuster stripGccModuleArguments() {
  return [](const clang::tooling::CommandLineArguments &Arguments,
            llvm::StringRef /*Filename*/) {
    clang::tooling::CommandLineArguments Result;
    Result.reserve(Arguments.size());

    bool SkipValue = false;
    for (const std::string &Argument : Arguments) {
      if (SkipValue) {
        SkipValue = false;
        continue;
      }

      const llvm::StringRef Value(Argument);
      if (Value == "-fmodules-ts" || Value.starts_with("-fmodule-mapper=") ||
          Value.starts_with("-fdeps-format=") ||
          Value.starts_with("-fdeps-file="))
        continue;
      if (Value == "-fmodule-mapper" || Value == "-fdeps-format" ||
          Value == "-fdeps-file") {
        SkipValue = true;
        continue;
      }
      Result.push_back(Argument);
    }
    return Result;
  };
}

} // namespace astrein
