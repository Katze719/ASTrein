#pragma once

#include "model/callback_signature.hpp"

#include "llvm/Support/JSON.h"

#include <string>
#include <unordered_map>

namespace astrein {

void patchFullAst(
    llvm::json::Value &Node,
    const std::unordered_map<std::string, CallbackSignature> &TypeSignatures,
    const std::unordered_map<std::string, CallbackSignature>
        &DeclarationSignatures);

} // namespace astrein
