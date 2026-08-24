#pragma once

#include <string>

namespace clang {
class Type;
}

namespace astrein {

[[nodiscard]] std::string pointerId(const clang::Type *Type);

} // namespace astrein
