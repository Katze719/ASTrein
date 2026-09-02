#pragma once

#include "model/enum_definition.hpp"
#include "model/struct_definition.hpp"

#include <string>
#include <vector>

namespace clang {
class ASTContext;
class FunctionDecl;
} // namespace clang

namespace astrein {

struct UsedTypes {
  std::vector<EnumDefinition> Enums;
  std::vector<StructDefinition> Structs;
};

[[nodiscard]] UsedTypes
usedTypes(const std::vector<const clang::FunctionDecl *> &Functions,
          clang::ASTContext &Context, const std::vector<std::string> &ApiRoots);

} // namespace astrein
