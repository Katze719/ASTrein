#include "ast/ast_frontend_action.hpp"

#include "ast/ast_consumer.hpp"

#include <utility>

namespace astrein {

AstFrontendAction::AstFrontendAction(llvm::raw_ostream &Output, OutputMode Mode,
                                     std::string PublicHeader, RunState &State,
                                     const std::vector<std::string> &ApiRoots)
    : Output(Output), Mode(Mode), PublicHeader(std::move(PublicHeader)),
      State(State), ApiRoots(ApiRoots) {}

std::unique_ptr<clang::ASTConsumer>
AstFrontendAction::CreateASTConsumer(clang::CompilerInstance &Compiler,
                                     llvm::StringRef /*InputFile*/) {
  return std::make_unique<AstConsumer>(Output, Mode, PublicHeader, State,
                                       Compiler, ApiRoots);
}

} // namespace astrein
