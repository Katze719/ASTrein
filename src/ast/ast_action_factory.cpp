#include "ast/ast_action_factory.hpp"

#include "ast/ast_frontend_action.hpp"

#include <utility>

namespace astrein {

AstActionFactory::AstActionFactory(llvm::raw_ostream &Output, OutputMode Mode,
                                   bool Minify, std::string PublicHeader,
                                   RunState &State,
                                   const std::vector<std::string> &ApiRoots,
                                   FfiFilter Filter)
    : Output(Output), Mode(Mode), Minify(Minify),
      PublicHeader(std::move(PublicHeader)), State(State), ApiRoots(ApiRoots),
      Filter(Filter) {}

std::unique_ptr<clang::FrontendAction> AstActionFactory::create() {
  return std::make_unique<AstFrontendAction>(Output, Mode, Minify, PublicHeader,
                                             State, ApiRoots, Filter);
}

} // namespace astrein
