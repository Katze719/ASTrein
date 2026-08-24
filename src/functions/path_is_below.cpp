#include "functions/path_is_below.hpp"

#include "functions/normalized_path.hpp"

namespace astrein {

bool pathIsBelow(llvm::StringRef Path, llvm::StringRef Root) {
  const std::string NormalPath = normalizedPath(Path);
  const std::string NormalRoot = normalizedPath(Root);
  return NormalPath == NormalRoot || (NormalPath.starts_with(NormalRoot) &&
                                      NormalPath.size() > NormalRoot.size() &&
                                      NormalPath[NormalRoot.size()] == '/');
}

} // namespace astrein
