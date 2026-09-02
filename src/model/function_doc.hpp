#pragma once

#include "model/declaration_doc.hpp"
#include "model/parameter_doc.hpp"

#include <string>
#include <unordered_map>

namespace astrein {

struct FunctionDoc : DeclarationDoc {
  std::string Returns;
  std::unordered_map<std::string, ParameterDoc> Parameters;
};

} // namespace astrein
