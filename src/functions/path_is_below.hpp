#pragma once

#include "llvm/ADT/StringRef.h"

namespace astrein {

[[nodiscard]] bool pathIsBelow(llvm::StringRef Path, llvm::StringRef Root);

} // namespace astrein
