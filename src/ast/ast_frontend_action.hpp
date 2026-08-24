#pragma once

#include "model/ffi_filter.hpp"
#include "model/output_mode.hpp"

#include "clang/Frontend/FrontendAction.h"

#include <memory>
#include <string>
#include <vector>

namespace llvm {
class raw_ostream;
}

namespace astrein {

struct RunState;

class AstFrontendAction final : public clang::ASTFrontendAction {
public:
  AstFrontendAction(llvm::raw_ostream &Output, OutputMode Mode,
                    std::string PublicHeader, RunState &State,
                    const std::vector<std::string> &ApiRoots, FfiFilter Filter);

  std::unique_ptr<clang::ASTConsumer>
  CreateASTConsumer(clang::CompilerInstance &Compiler,
                    llvm::StringRef InputFile) override;

private:
  llvm::raw_ostream &Output;
  OutputMode Mode;
  std::string PublicHeader;
  RunState &State;
  const std::vector<std::string> &ApiRoots;
  FfiFilter Filter;
};

} // namespace astrein
