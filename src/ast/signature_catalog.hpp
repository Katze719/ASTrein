#pragma once

#include "model/callback_signature.hpp"

#include "clang/AST/PrettyPrinter.h"

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace clang {
class ASTContext;
class Decl;
class DeclaratorDecl;
class FunctionProtoType;
class ParmVarDecl;
class QualType;
class Type;
class TypeSourceInfo;
class TypedefNameDecl;
} // namespace clang

namespace astrein {

class SignatureCatalog {
public:
  SignatureCatalog(clang::ASTContext &Context, clang::PrintingPolicy Policy);

  void collect(clang::DeclaratorDecl *Declaration);
  void collect(clang::TypedefNameDecl *Declaration);
  void finalizeDeclarations();

  [[nodiscard]] const CallbackSignature *
  lookup(const clang::ParmVarDecl &Parameter) const;
  [[nodiscard]] const std::unordered_map<std::string, CallbackSignature> &
  byDeclarationJsonId() const;
  [[nodiscard]] const std::unordered_map<std::string, CallbackSignature> &
  byJsonId() const;

private:
  [[nodiscard]] static std::string
  pointerIdForDecl(const clang::Decl *Declaration);
  [[nodiscard]] std::optional<CallbackSignature>
  collectTypeSourceInfo(clang::TypeSourceInfo *Info);
  [[nodiscard]] const CallbackSignature *
  lookup(clang::QualType CallbackType) const;
  void add(const clang::FunctionProtoType *Function,
           CallbackSignature Signature);

  clang::ASTContext &Context;
  clang::PrintingPolicy Policy;
  std::unordered_map<const clang::FunctionProtoType *, CallbackSignature>
      ByExactType;
  std::unordered_map<const clang::Type *, std::vector<CallbackSignature>>
      ByCanonicalType;
  std::unordered_map<const clang::TypedefNameDecl *, CallbackSignature>
      ByTypedef;
  std::vector<const clang::ParmVarDecl *> Parameters;
  std::unordered_map<std::string, CallbackSignature> ByJsonId;
  std::unordered_map<std::string, CallbackSignature> ByDeclarationJsonId;
};

} // namespace astrein
