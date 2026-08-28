#include "ast/ast_frontend_action.hpp"

#include "ast/ast_consumer.hpp"

#include <utility>

namespace astrein {

AstFrontendAction::AstFrontendAction(llvm::raw_ostream &Output, OutputMode Mode,
                                     bool Minify, std::string PublicHeader,
                                     RunState &State,
                                     const std::vector<std::string> &ApiRoots,
                                     FfiFilter Filter)
    : Output(Output), Mode(Mode), Minify(Minify),
      PublicHeader(std::move(PublicHeader)), State(State), ApiRoots(ApiRoots),
      Filter(Filter) {}

std::unique_ptr<clang::ASTConsumer>
AstFrontendAction::CreateASTConsumer(clang::CompilerInstance &Compiler,
                                     llvm::StringRef /*InputFile*/) {
  return std::make_unique<AstConsumer>(Output, Mode, Minify, PublicHeader,
                                       State, Compiler, ApiRoots, Filter);
}

} // namespace astrein
