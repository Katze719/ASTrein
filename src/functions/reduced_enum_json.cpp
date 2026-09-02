#include "functions/reduced_enum_json.hpp"

#include "functions/declared_in.hpp"
#include "functions/documentation_json.hpp"
#include "functions/extract_documentation.hpp"
#include "functions/print_type.hpp"
#include "model/declaration_doc.hpp"
#include "model/enum_definition.hpp"

#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "llvm/ADT/StringExtras.h"

#include <cstdint>
#include <iterator>
#include <optional>

namespace astrein {

llvm::json::Object reducedEnumJson(const EnumDefinition &Definition,
                                   clang::ASTContext &Context,
                                   const clang::PrintingPolicy &Policy,
                                   const std::vector<std::string> &ApiRoots) {
  const clang::EnumDecl &Declaration = *Definition.Declaration;
  llvm::json::Object JsonEnum;
  JsonEnum["name"] = Definition.Name;
  JsonEnum["declaredIn"] =
      declaredIn(Declaration, Context.getSourceManager(), ApiRoots);
  JsonEnum["scoped"] = Declaration.isScoped();

  llvm::json::Object JsonDocs =
      documentationJson(extractDocumentation(Declaration, Context));
  if (!JsonDocs.empty())
    JsonEnum["doc"] = std::move(JsonDocs);

  if (!Definition.Aliases.empty()) {
    llvm::json::Array Aliases;
    Aliases.reserve(Definition.Aliases.size());
    for (const std::string &Alias : Definition.Aliases)
      Aliases.emplace_back(Alias);
    JsonEnum["aliases"] = std::move(Aliases);
  }

  const clang::QualType IntegerType = Declaration.getIntegerType();
  if (!IntegerType.isNull()) {
    JsonEnum["underlyingType"] = printType(IntegerType, Policy);
    const clang::QualType EnumType = Context.getTypeDeclType(
        clang::ElaboratedTypeKeyword::None, std::nullopt, &Declaration);
    const std::optional<clang::CharUnits> Size =
        Context.getTypeSizeInCharsIfKnown(EnumType);
    if (Size.has_value()) {
      JsonEnum["size"] = static_cast<int64_t>(Size->getQuantity());
      JsonEnum["alignment"] = static_cast<int64_t>(
          Context.getTypeAlignInChars(EnumType).getQuantity());
    }
  }

  llvm::json::Array Values;
  if (Declaration.isCompleteDefinition()) {
    Values.reserve(std::distance(Declaration.enumerator_begin(),
                                 Declaration.enumerator_end()));
    for (const clang::EnumConstantDecl *Enumerator :
         Declaration.enumerators()) {
      llvm::json::Object JsonValue;
      JsonValue["name"] = Enumerator->getNameAsString();
      JsonValue["value"] = llvm::toString(Enumerator->getInitVal(), 10);
      llvm::json::Object JsonValueDocs =
          documentationJson(extractDocumentation(*Enumerator, Context));
      if (!JsonValueDocs.empty())
        JsonValue["doc"] = std::move(JsonValueDocs);
      Values.emplace_back(std::move(JsonValue));
    }
  } else {
    JsonEnum["opaque"] = true;
  }
  JsonEnum["values"] = std::move(Values);
  return JsonEnum;
}

} // namespace astrein
