#pragma once

#include "llvm/ADT/StringRef.h"

#include <string>

namespace astrein {

[[nodiscard]] std::string normalizedPath(llvm::StringRef Path);

} // namespace astrein
