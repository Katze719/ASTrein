#pragma once

#include "model/output_mode.hpp"

#include "clang/Tooling/Tooling.h"

#include <memory>
#include <string>
#include <vector>

namespace llvm {
class raw_ostream;
}

namespace astrein {

struct RunState;

class AstActionFactory final : public clang::tooling::FrontendActionFactory {
public:
  AstActionFactory(llvm::raw_ostream &Output, OutputMode Mode,
                   std::string PublicHeader, RunState &State,
                   const std::vector<std::string> &ApiRoots);

  std::unique_ptr<clang::FrontendAction> create() override;

private:
  llvm::raw_ostream &Output;
  OutputMode Mode;
  std::string PublicHeader;
  RunState &State;
  const std::vector<std::string> &ApiRoots;
};

} // namespace astrein
