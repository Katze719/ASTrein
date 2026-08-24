#include "functions/function_prototype.hpp"

namespace astrein {

const clang::FunctionProtoType *functionPrototype(clang::QualType Type) {
  for (unsigned Depth = 0; Depth != 32 && !Type.isNull(); ++Depth) {
    if (const auto *Function = Type->getAs<clang::FunctionProtoType>())
      return Function;
    if (const auto *Pointer = Type->getAs<clang::PointerType>()) {
      Type = Pointer->getPointeeType();
      continue;
    }
    if (const auto *Reference = Type->getAs<clang::ReferenceType>()) {
      Type = Reference->getPointeeType();
      continue;
    }
    if (const auto *MemberPointer = Type->getAs<clang::MemberPointerType>()) {
      Type = MemberPointer->getPointeeType();
      continue;
    }
    if (const auto *BlockPointer = Type->getAs<clang::BlockPointerType>()) {
      Type = BlockPointer->getPointeeType();
      continue;
    }
    break;
  }
  return nullptr;
}

} // namespace astrein
