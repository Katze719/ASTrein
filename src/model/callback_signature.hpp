#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace astrein {

struct CallbackParameter {
  std::string Type;
  std::optional<std::string> Name;
  std::optional<int64_t> Size;
  std::optional<int64_t> Alignment;
};

struct CallbackSignature {
  std::string ReturnType;
  std::vector<CallbackParameter> Parameters;

  [[nodiscard]] std::size_t namedParameterCount() const;
};

} // namespace astrein
