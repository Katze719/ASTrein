#include "functions/append_words.hpp"

namespace astrein {

std::string appendWords(std::string Current, llvm::StringRef More) {
  More = More.trim();
  if (More.empty())
    return Current;
  if (!Current.empty())
    Current.push_back(' ');
  Current.append(More.begin(), More.end());
  return Current;
}

} // namespace astrein
