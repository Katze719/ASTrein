#include "model/callback_signature.hpp"

#include <algorithm>
#include <ranges>

namespace astrein {

std::size_t CallbackSignature::namedParameterCount() const {
  return std::ranges::count_if(Parameters, [](const auto &Parameter) {
    return Parameter.Name.has_value();
  });
}

} // namespace astrein
