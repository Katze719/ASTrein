#include "functions/callback_parameters_json.hpp"

#include "model/callback_signature.hpp"

namespace astrein {

llvm::json::Array callbackParametersJson(const CallbackSignature &Signature,
                                         bool ClangTypeObjects) {
  llvm::json::Array Parameters;
  Parameters.reserve(Signature.ParameterTypes.size());
  for (std::size_t Index = 0; Index != Signature.ParameterTypes.size();
       ++Index) {
    llvm::json::Object Parameter;
    if (Index < Signature.ParameterNames.size() &&
        Signature.ParameterNames[Index].has_value())
      Parameter["name"] = *Signature.ParameterNames[Index];
    if (ClangTypeObjects) {
      llvm::json::Object Type;
      Type["qualType"] = Signature.ParameterTypes[Index];
      Parameter["type"] = std::move(Type);
    } else {
      Parameter["type"] = Signature.ParameterTypes[Index];
    }
    Parameters.emplace_back(std::move(Parameter));
  }
  return Parameters;
}

} // namespace astrein
