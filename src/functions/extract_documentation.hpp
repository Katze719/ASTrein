#pragma once

#include "model/function_doc.hpp"

namespace clang {
class ASTContext;
class FunctionDecl;
} // namespace clang

namespace astrein {

[[nodiscard]] FunctionDoc
extractDocumentation(const clang::FunctionDecl &Declaration,
                     clang::ASTContext &Context);

} // namespace astrein
