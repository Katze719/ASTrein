#pragma once

#include <string>
#include <vector>

namespace clang {
class EnumDecl;
}

namespace astrein {

struct EnumDefinition {
  const clang::EnumDecl *Declaration = nullptr;
  std::string Name;
  std::vector<std::string> Aliases;
};

} // namespace astrein
