#pragma once

#include "clang/Tooling/ArgumentsAdjusters.h"

namespace astrein {

[[nodiscard]] clang::tooling::ArgumentsAdjuster stripGccModuleArguments();

} // namespace astrein
