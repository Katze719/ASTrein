#pragma once

#include "llvm/ADT/StringRef.h"

#include <string>

namespace astrein {

[[nodiscard]] std::string appendWords(std::string Current,
                                      llvm::StringRef More);

} // namespace astrein
