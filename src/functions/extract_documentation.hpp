#pragma once

#include "model/function_doc.hpp"

namespace clang {
class ASTContext;
class Decl;
class FunctionDecl;
} // namespace clang

namespace astrein {

struct DeclarationDoc;

[[nodiscard]] DeclarationDoc
extractDocumentation(const clang::Decl &Declaration,
                     clang::ASTContext &Context);

[[nodiscard]] FunctionDoc
extractDocumentation(const clang::FunctionDecl &Declaration,
                     clang::ASTContext &Context);

} // namespace astrein
