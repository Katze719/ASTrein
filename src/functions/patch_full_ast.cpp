#include "functions/patch_full_ast.hpp"

#include "functions/callback_parameters_json.hpp"

namespace astrein {

void patchFullAst(
    llvm::json::Value &Node,
    const std::unordered_map<std::string, CallbackSignature> &TypeSignatures,
    const std::unordered_map<std::string, CallbackSignature>
        &DeclarationSignatures) {
  if (auto *Object = Node.getAsObject()) {
    const auto Kind = Object->getString("kind");
    const auto Id = Object->getString("id");
    if (Kind && Id && *Kind == "FunctionProtoType") {
      if (const auto Signature = TypeSignatures.find(Id->str());
          Signature != TypeSignatures.end())
        (*Object)["parameters"] =
            callbackParametersJson(Signature->second, true);
    }
    if (Id) {
      if (const auto Signature = DeclarationSignatures.find(Id->str());
          Signature != DeclarationSignatures.end())
        (*Object)["callbackParameters"] =
            callbackParametersJson(Signature->second, true);
    }
    for (auto &[Key, Child] : *Object) {
      (void)Key;
      patchFullAst(Child, TypeSignatures, DeclarationSignatures);
    }
    return;
  }

  if (auto *Array = Node.getAsArray())
    for (auto &Child : *Array)
      patchFullAst(Child, TypeSignatures, DeclarationSignatures);
}

} // namespace astrein
