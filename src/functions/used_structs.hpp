#pragma once

#include "model/struct_definition.hpp"

#include <string>
#include <vector>

namespace clang {
class ASTContext;
class FunctionDecl;
}

namespace astrein {

[[nodiscard]] std::vector<StructDefinition> usedStructs(
    const std::vector<const clang::FunctionDecl *> &Functions,
    clang::ASTContext &Context, const std::vector<std::string> &ApiRoots);

} // namespace astrein
