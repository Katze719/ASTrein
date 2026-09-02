#pragma once

#include "llvm/Support/JSON.h"

namespace astrein {

struct DeclarationDoc;
struct FunctionDoc;

[[nodiscard]] llvm::json::Object documentationJson(const DeclarationDoc &Doc);
[[nodiscard]] llvm::json::Object documentationJson(const FunctionDoc &Doc);

} // namespace astrein
