#pragma once

#include "llvm/Support/JSON.h"

#include <string>
#include <vector>

namespace clang {
class ASTContext;
class ASTNameGenerator;
class FunctionDecl;
class PrintingPolicy;
} // namespace clang

namespace astrein {

class SignatureCatalog;

[[nodiscard]] llvm::json::Object reducedFunctionJson(
    const clang::FunctionDecl &Declaration, clang::ASTContext &Context,
    const SignatureCatalog &Signatures, clang::ASTNameGenerator &Names,
    const clang::PrintingPolicy &Policy,
    const std::vector<std::string> &ApiRoots);

} // namespace astrein
