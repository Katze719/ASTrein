#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace astrein {

struct CallbackSignature {
  std::string ReturnType;
  std::vector<std::string> ParameterTypes;
  std::vector<std::optional<std::string>> ParameterNames;

  [[nodiscard]] std::size_t namedParameterCount() const;
};

} // namespace astrein
