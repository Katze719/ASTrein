#include "functions/declared_in.hpp"

#include "functions/api_relative_path.hpp"

#include "clang/AST/Decl.h"
#include "clang/Basic/SourceManager.h"

namespace astrein {

std::string declaredIn(const clang::FunctionDecl &Declaration,
                       const clang::SourceManager &Sources,
                       const std::vector<std::string> &ApiRoots) {
  const clang::PresumedLoc Location =
      Sources.getPresumedLoc(Declaration.getLocation());
  return Location.isValid() ? apiRelativePath(Location.getFilename(), ApiRoots)
                            : std::string{};
}

} // namespace astrein
