#include "ast/ast_action_factory.hpp"

#include "ast/ast_frontend_action.hpp"

#include <utility>

namespace astrein {

AstActionFactory::AstActionFactory(llvm::raw_ostream &Output, OutputMode Mode,
                                   std::string PublicHeader, RunState &State,
                                   const std::vector<std::string> &ApiRoots)
    : Output(Output), Mode(Mode), PublicHeader(std::move(PublicHeader)),
      State(State), ApiRoots(ApiRoots) {}

std::unique_ptr<clang::FrontendAction> AstActionFactory::create() {
  return std::make_unique<AstFrontendAction>(Output, Mode, PublicHeader, State,
                                             ApiRoots);
}

} // namespace astrein
