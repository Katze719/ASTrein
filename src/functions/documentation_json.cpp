#include "functions/documentation_json.hpp"

#include "model/declaration_doc.hpp"
#include "model/function_doc.hpp"

namespace astrein {

llvm::json::Object documentationJson(const DeclarationDoc &Doc) {
  llvm::json::Object Result;
  if (!Doc.Brief.empty())
    Result["brief"] = Doc.Brief;
  if (!Doc.Details.empty()) {
    llvm::json::Array Details;
    for (const auto &Detail : Doc.Details)
      Details.emplace_back(Detail);
    Result["details"] = std::move(Details);
  }
  return Result;
}

llvm::json::Object documentationJson(const FunctionDoc &Doc) {
  llvm::json::Object Result =
      documentationJson(static_cast<const DeclarationDoc &>(Doc));
  if (!Doc.Returns.empty())
    Result["returns"] = Doc.Returns;
  return Result;
}

} // namespace astrein
