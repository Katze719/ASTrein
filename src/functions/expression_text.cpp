#include "functions/expression_text.hpp"

#include "clang/AST/Expr.h"
#include "clang/AST/PrettyPrinter.h"
#include "llvm/Support/raw_ostream.h"

namespace astrein {

std::string expressionText(const clang::Expr &Expression,
                           const clang::PrintingPolicy &Policy) {
  std::string Result;
  llvm::raw_string_ostream Stream(Result);
  Expression.printPretty(Stream, nullptr, Policy);
  return Result;
}

} // namespace astrein
