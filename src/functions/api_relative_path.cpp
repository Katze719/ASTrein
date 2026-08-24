#include "functions/api_relative_path.hpp"

#include "functions/normalized_path.hpp"
#include "functions/path_is_below.hpp"

namespace astrein {

std::string apiRelativePath(llvm::StringRef Path,
                            const std::vector<std::string> &ApiRoots) {
  const std::string NormalPath = normalizedPath(Path);
  for (const std::string &Root : ApiRoots) {
    const std::string NormalRoot = normalizedPath(Root);
    if (!pathIsBelow(NormalPath, NormalRoot))
      continue;
    if (NormalPath.size() == NormalRoot.size())
      return {};
    return NormalPath.substr(NormalRoot.size() + 1);
  }
  return NormalPath;
}

} // namespace astrein
