#pragma once

#include "llvm/Support/JSON.h"

#include <string>
#include <vector>

namespace clang {
class ASTContext;
class PrintingPolicy;
} // namespace clang

namespace astrein {

struct EnumDefinition;

[[nodiscard]] llvm::json::Object
reducedEnumJson(const EnumDefinition &Definition, clang::ASTContext &Context,
                const clang::PrintingPolicy &Policy,
                const std::vector<std::string> &ApiRoots);

} // namespace astrein
