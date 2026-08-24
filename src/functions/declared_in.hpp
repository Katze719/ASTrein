#pragma once

#include <string>
#include <vector>

namespace clang {
class FunctionDecl;
class SourceManager;
} // namespace clang

namespace astrein {

[[nodiscard]] std::string declaredIn(const clang::FunctionDecl &Declaration,
                                     const clang::SourceManager &Sources,
                                     const std::vector<std::string> &ApiRoots);

} // namespace astrein
