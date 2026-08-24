#include "functions/normalized_path.hpp"

#include <algorithm>
#include <cctype>
#include <ranges>

namespace astrein {

std::string normalizedPath(llvm::StringRef Path) {
  std::string Result = Path.str();
  std::ranges::replace(Result, '\\', '/');
  while (Result.size() > 1 && Result.back() == '/')
    Result.pop_back();
#ifdef _WIN32
  std::ranges::transform(Result, Result.begin(), [](unsigned char Character) {
    return static_cast<char>(std::tolower(Character));
  });
#endif
  return Result;
}

} // namespace astrein
