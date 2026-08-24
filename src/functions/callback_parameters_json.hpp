#pragma once

#include "llvm/Support/JSON.h"

namespace astrein {

struct CallbackSignature;

[[nodiscard]] llvm::json::Array
callbackParametersJson(const CallbackSignature &Signature,
                       bool ClangTypeObjects);

} // namespace astrein
