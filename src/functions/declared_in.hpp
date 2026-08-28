#pragma once

#include <string>
#include <vector>

namespace clang {
class Decl;
class SourceManager;
} // namespace clang

namespace astrein {

[[nodiscard]] std::string declaredIn(const clang::Decl &Declaration,
                                     const clang::SourceManager &Sources,
                                     const std::vector<std::string> &ApiRoots);

} // namespace astrein
