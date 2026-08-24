#include "model/callback_signature.hpp"

#include <algorithm>
#include <ranges>

namespace astrein {

std::size_t CallbackSignature::namedParameterCount() const {
  return std::ranges::count_if(
      ParameterNames, [](const auto &Name) { return Name.has_value(); });
}

} // namespace astrein
