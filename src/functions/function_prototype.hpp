#pragma once

#include "clang/AST/Type.h"

namespace astrein {

[[nodiscard]] const clang::FunctionProtoType *
functionPrototype(clang::QualType Type);

} // namespace astrein
