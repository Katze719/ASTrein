#include "functions/callback_json.hpp"

#include "functions/callback_parameters_json.hpp"
#include "model/callback_signature.hpp"

namespace astrein {

llvm::json::Object callbackJson(const CallbackSignature &Signature) {
  llvm::json::Object Callback;
  Callback["returnType"] = Signature.ReturnType;
  Callback["parameters"] = callbackParametersJson(Signature, false);
  return Callback;
}

} // namespace astrein
