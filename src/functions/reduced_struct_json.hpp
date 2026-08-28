#pragma once

#include "llvm/Support/JSON.h"

#include <string>
#include <vector>

namespace clang {
class ASTContext;
class PrintingPolicy;
}

namespace astrein {

struct StructDefinition;

[[nodiscard]] llvm::json::Object reducedStructJson(
    const StructDefinition &Definition, clang::ASTContext &Context,
    const clang::PrintingPolicy &Policy,
    const std::vector<std::string> &ApiRoots);

} // namespace astrein
