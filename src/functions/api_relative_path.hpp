#pragma once

#include "llvm/ADT/StringRef.h"

#include <string>
#include <vector>

namespace astrein {

[[nodiscard]] std::string
apiRelativePath(llvm::StringRef Path, const std::vector<std::string> &ApiRoots);

} // namespace astrein
