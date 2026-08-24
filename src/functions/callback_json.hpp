#pragma once

#include "llvm/Support/JSON.h"

namespace astrein {

struct CallbackSignature;

[[nodiscard]] llvm::json::Object
callbackJson(const CallbackSignature &Signature);

} // namespace astrein
