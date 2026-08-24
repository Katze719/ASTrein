#include "functions/pointer_id.hpp"

#include "llvm/Support/raw_ostream.h"

namespace astrein {

std::string pointerId(const clang::Type *Type) {
  std::string Result;
  llvm::raw_string_ostream Stream(Result);
  Stream << static_cast<const void *>(Type);
  return Result;
}

} // namespace astrein
