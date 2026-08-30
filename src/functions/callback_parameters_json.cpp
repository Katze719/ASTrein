#include "functions/callback_parameters_json.hpp"

#include "model/callback_signature.hpp"

namespace astrein {

llvm::json::Array callbackParametersJson(const CallbackSignature &Signature,
                                         bool ClangTypeObjects) {
  llvm::json::Array Parameters;
  Parameters.reserve(Signature.Parameters.size());
  for (const CallbackParameter &ParameterInfo : Signature.Parameters) {
    llvm::json::Object JsonParameter;
    if (ParameterInfo.Name.has_value())
      JsonParameter["name"] = *ParameterInfo.Name;
    if (ClangTypeObjects) {
      llvm::json::Object Type;
      Type["qualType"] = ParameterInfo.Type;
      JsonParameter["type"] = std::move(Type);
    } else {
      JsonParameter["type"] = ParameterInfo.Type;
      if (ParameterInfo.Size.has_value())
        JsonParameter["size"] = *ParameterInfo.Size;
      if (ParameterInfo.Alignment.has_value())
        JsonParameter["alignment"] = *ParameterInfo.Alignment;
    }
    Parameters.emplace_back(std::move(JsonParameter));
  }
  return Parameters;
}

} // namespace astrein
