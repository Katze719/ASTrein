#include "functions/reduced_struct_json.hpp"

#include "functions/declared_in.hpp"
#include "functions/documentation_json.hpp"
#include "functions/expression_text.hpp"
#include "functions/extract_documentation.hpp"
#include "functions/print_type.hpp"
#include "model/declaration_doc.hpp"
#include "model/struct_definition.hpp"

#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/RecordLayout.h"

#include <cstdint>
#include <iterator>
#include <optional>

namespace astrein {

llvm::json::Object reducedStructJson(const StructDefinition &Definition,
                                     clang::ASTContext &Context,
                                     const clang::PrintingPolicy &Policy,
                                     const std::vector<std::string> &ApiRoots) {
  const clang::RecordDecl &Declaration = *Definition.Declaration;
  llvm::json::Object JsonStruct;
  JsonStruct["name"] = Definition.Name;
  JsonStruct["declaredIn"] =
      declaredIn(Declaration, Context.getSourceManager(), ApiRoots);

  llvm::json::Object JsonDocs =
      documentationJson(extractDocumentation(Declaration, Context));
  if (!JsonDocs.empty())
    JsonStruct["doc"] = std::move(JsonDocs);

  if (!Definition.Aliases.empty()) {
    llvm::json::Array Aliases;
    Aliases.reserve(Definition.Aliases.size());
    for (const std::string &Alias : Definition.Aliases)
      Aliases.emplace_back(Alias);
    JsonStruct["aliases"] = std::move(Aliases);
  }

  llvm::json::Array Fields;
  if (Declaration.isCompleteDefinition()) {
    const clang::ASTRecordLayout &Layout =
        Context.getASTRecordLayout(&Declaration);
    JsonStruct["size"] = static_cast<int64_t>(Layout.getSize().getQuantity());
    JsonStruct["alignment"] =
        static_cast<int64_t>(Layout.getAlignment().getQuantity());

    Fields.reserve(
        std::distance(Declaration.field_begin(), Declaration.field_end()));
    unsigned FieldIndex = 0;
    for (const clang::FieldDecl *Field : Declaration.fields()) {
      llvm::json::Object JsonField;
      JsonField["name"] = Field->getNameAsString();
      JsonField["type"] = printType(Field->getType(), Policy);
      const uint64_t OffsetBits = Layout.getFieldOffset(FieldIndex++);
      JsonField["offset"] =
          static_cast<int64_t>(OffsetBits / Context.getCharWidth());
      const std::optional<clang::CharUnits> FieldSize =
          Context.getTypeSizeInCharsIfKnown(Field->getType());
      JsonField["size"] = static_cast<int64_t>(
          FieldSize.has_value() ? FieldSize->getQuantity() : 0);
      if (Field->isBitField() && Field->getBitWidth() != nullptr) {
        JsonField["bitOffset"] = static_cast<int64_t>(OffsetBits);
        JsonField["bitWidth"] = expressionText(*Field->getBitWidth(), Policy);
      }
      llvm::json::Object JsonFieldDocs =
          documentationJson(extractDocumentation(*Field, Context));
      if (!JsonFieldDocs.empty())
        JsonField["doc"] = std::move(JsonFieldDocs);
      Fields.emplace_back(std::move(JsonField));
    }
  } else {
    JsonStruct["opaque"] = true;
  }
  JsonStruct["fields"] = std::move(Fields);
  return JsonStruct;
}

} // namespace astrein
