#pragma once

#include "model/parameter_doc.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace astrein {

struct FunctionDoc {
  std::string Brief;
  std::string Returns;
  std::vector<std::string> Details;
  std::unordered_map<std::string, ParameterDoc> Parameters;
};

} // namespace astrein
