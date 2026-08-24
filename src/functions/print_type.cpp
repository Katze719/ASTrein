#include "functions/print_type.hpp"

#include "clang/AST/PrettyPrinter.h"

namespace astrein {

std::string printType(clang::QualType Type,
                      const clang::PrintingPolicy &Policy) {
  return Type.getAsString(Policy);
}

} // namespace astrein
