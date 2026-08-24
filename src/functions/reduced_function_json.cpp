#include "functions/reduced_function_json.hpp"

#include "ast/signature_catalog.hpp"
#include "functions/callback_json.hpp"
#include "functions/declared_in.hpp"
#include "functions/documentation_json.hpp"
#include "functions/expression_text.hpp"
#include "functions/extract_documentation.hpp"
#include "functions/print_type.hpp"
#include "model/function_doc.hpp"

#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Mangle.h"

namespace astrein {

llvm::json::Object reducedFunctionJson(
    const clang::FunctionDecl &Declaration, clang::ASTContext &Context,
    const SignatureCatalog &Signatures, clang::ASTNameGenerator &Names,
    const clang::PrintingPolicy &Policy,
    const std::vector<std::string> &ApiRoots) {
  llvm::json::Object Function;
  Function["name"] = Declaration.getQualifiedNameAsString();
  Function["symbol"] = Names.getName(&Declaration);
  Function["declaredIn"] =
      declaredIn(Declaration, Context.getSourceManager(), ApiRoots);
  Function["returnType"] = printType(Declaration.getReturnType(), Policy);

  const FunctionDoc Docs = extractDocumentation(Declaration, Context);
  llvm::json::Array Parameters;
  Parameters.reserve(Declaration.getNumParams());
  for (const clang::ParmVarDecl *Parameter : Declaration.parameters()) {
    llvm::json::Object JsonParameter;
    JsonParameter["name"] = Parameter->getNameAsString();
    JsonParameter["type"] = printType(Parameter->getType(), Policy);

    if (Parameter->hasDefaultArg() && Parameter->getDefaultArg() != nullptr)
      JsonParameter["default"] =
          expressionText(*Parameter->getDefaultArg(), Policy);

    if (const CallbackSignature *Callback = Signatures.lookup(*Parameter))
      JsonParameter["callback"] = callbackJson(*Callback);

    if (const auto Doc = Docs.Parameters.find(Parameter->getNameAsString());
        Doc != Docs.Parameters.end()) {
      llvm::json::Object JsonDoc;
      if (!Doc->second.Description.empty())
        JsonDoc["description"] = Doc->second.Description;
      JsonDoc["direction"] = Doc->second.Direction;
      JsonParameter["doc"] = std::move(JsonDoc);
    }
    Parameters.emplace_back(std::move(JsonParameter));
  }
  Function["parameters"] = std::move(Parameters);

  if (Declaration.isInlineSpecified() || Declaration.isInlined())
    Function["inline"] = true;

  llvm::json::Object JsonDocs = documentationJson(Docs);
  if (!JsonDocs.empty())
    Function["doc"] = std::move(JsonDocs);
  return Function;
}

} // namespace astrein
