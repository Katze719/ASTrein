#pragma once

#include <string>

namespace clang {
class Expr;
class PrintingPolicy;
} // namespace clang

namespace astrein {

[[nodiscard]] std::string expressionText(const clang::Expr &Expression,
                                         const clang::PrintingPolicy &Policy);

} // namespace astrein
