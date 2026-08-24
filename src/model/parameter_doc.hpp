#pragma once

#include <string>

namespace astrein {

struct ParameterDoc {
  std::string Description;
  std::string Direction = "in";
};

} // namespace astrein
