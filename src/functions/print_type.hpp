#pragma once

#include "clang/AST/Type.h"

#include <string>

namespace clang {
class PrintingPolicy;
}

namespace astrein {

[[nodiscard]] std::string printType(clang::QualType Type,
                                    const clang::PrintingPolicy &Policy);

} // namespace astrein
