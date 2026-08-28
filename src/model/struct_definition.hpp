#pragma once

#include <string>
#include <vector>

namespace clang {
class RecordDecl;
}

namespace astrein {

struct StructDefinition {
  const clang::RecordDecl *Declaration = nullptr;
  std::string Name;
  std::vector<std::string> Aliases;
};

} // namespace astrein
