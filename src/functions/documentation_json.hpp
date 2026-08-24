#pragma once

#include "llvm/Support/JSON.h"

namespace astrein {

struct FunctionDoc;

[[nodiscard]] llvm::json::Object documentationJson(const FunctionDoc &Doc);

} // namespace astrein
