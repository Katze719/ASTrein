#include "ast/signature_catalog.hpp"

#include "functions/function_prototype.hpp"
#include "functions/pointer_id.hpp"
#include "functions/print_type.hpp"

#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Type.h"
#include "clang/AST/TypeLoc.h"
#include "llvm/Support/raw_ostream.h"

#include <algorithm>
#include <ranges>
#include <utility>

namespace astrein {

SignatureCatalog::SignatureCatalog(clang::ASTContext &Context,
                                   clang::PrintingPolicy Policy)
    : Context(Context), Policy(std::move(Policy)) {}

void SignatureCatalog::collect(clang::DeclaratorDecl *Declaration) {
  if (Declaration == nullptr)
    return;
  std::optional<CallbackSignature> Direct =
      collectTypeSourceInfo(Declaration->getTypeSourceInfo());
  if (auto *Parameter = llvm::dyn_cast<clang::ParmVarDecl>(Declaration)) {
    Parameters.push_back(Parameter);
    if (Direct.has_value() &&
        functionPrototype(Parameter->getType()) != nullptr)
      ByDeclarationJsonId.insert_or_assign(pointerIdForDecl(Parameter),
                                           std::move(*Direct));
  }
}

void SignatureCatalog::collect(clang::TypedefNameDecl *Declaration) {
  if (Declaration == nullptr)
    return;
  std::optional<CallbackSignature> Signature =
      collectTypeSourceInfo(Declaration->getTypeSourceInfo());
  if (!Signature.has_value())
    return;
  ByTypedef.insert_or_assign(Declaration, *Signature);
  ByDeclarationJsonId.insert_or_assign(pointerIdForDecl(Declaration),
                                       std::move(*Signature));
}

void SignatureCatalog::finalizeDeclarations() {
  for (const clang::ParmVarDecl *Parameter : Parameters) {
    if (const CallbackSignature *Signature = lookup(*Parameter))
      ByDeclarationJsonId.insert_or_assign(pointerIdForDecl(Parameter),
                                           *Signature);
  }
}

const CallbackSignature *
SignatureCatalog::lookup(const clang::ParmVarDecl &Parameter) const {
  if (const auto Direct =
          ByDeclarationJsonId.find(pointerIdForDecl(&Parameter));
      Direct != ByDeclarationJsonId.end())
    return &Direct->second;

  clang::QualType Current = Parameter.getType();
  for (unsigned Depth = 0; Depth != 32 && !Current.isNull(); ++Depth) {
    const clang::Type *Raw = Current.getTypePtr();
    if (const auto *Typedef = llvm::dyn_cast<clang::TypedefType>(Raw)) {
      if (const auto Found = ByTypedef.find(Typedef->getDecl());
          Found != ByTypedef.end())
        return &Found->second;
    }
    clang::QualType Desugared = Current.getSingleStepDesugaredType(Context);
    if (Desugared == Current)
      break;
    Current = Desugared;
  }
  return lookup(Parameter.getType());
}

const std::unordered_map<std::string, CallbackSignature> &
SignatureCatalog::byDeclarationJsonId() const {
  return ByDeclarationJsonId;
}

const std::unordered_map<std::string, CallbackSignature> &
SignatureCatalog::byJsonId() const {
  return ByJsonId;
}

std::string SignatureCatalog::pointerIdForDecl(const clang::Decl *Declaration) {
  std::string Result;
  llvm::raw_string_ostream Stream(Result);
  Stream << static_cast<const void *>(Declaration);
  return Result;
}

std::optional<CallbackSignature>
SignatureCatalog::collectTypeSourceInfo(clang::TypeSourceInfo *Info) {
  if (Info == nullptr)
    return std::nullopt;

  std::optional<CallbackSignature> Found;
  for (clang::TypeLoc Location = Info->getTypeLoc(); !Location.isNull();
       Location = Location.getNextTypeLoc()) {
    const auto FunctionLocation = Location.getAs<clang::FunctionProtoTypeLoc>();
    if (FunctionLocation.isNull())
      continue;

    const auto *Function = FunctionLocation.getTypePtr();
    CallbackSignature Signature;
    Signature.ReturnType = printType(Function->getReturnType(), Policy);
    Signature.ParameterTypes.reserve(Function->getNumParams());
    Signature.ParameterNames.reserve(Function->getNumParams());

    for (unsigned Index = 0; Index != Function->getNumParams(); ++Index) {
      Signature.ParameterTypes.push_back(
          printType(Function->getParamType(Index), Policy));
      const clang::ParmVarDecl *Parameter = FunctionLocation.getParam(Index);
      if (Parameter != nullptr && !Parameter->getName().empty())
        Signature.ParameterNames.emplace_back(Parameter->getNameAsString());
      else
        Signature.ParameterNames.emplace_back(std::nullopt);
    }

    Found = Signature;
    add(Function, std::move(Signature));
  }
  return Found;
}

const CallbackSignature *
SignatureCatalog::lookup(clang::QualType CallbackType) const {
  const clang::FunctionProtoType *Function = functionPrototype(CallbackType);
  if (Function == nullptr)
    return nullptr;

  if (const auto Exact = ByExactType.find(Function); Exact != ByExactType.end())
    return &Exact->second;

  const clang::Type *Canonical = clang::ASTContext::getCanonicalType(Function);
  const auto Candidates = ByCanonicalType.find(Canonical);
  if (Candidates == ByCanonicalType.end() || Candidates->second.empty())
    return nullptr;
  return &Candidates->second.front();
}

void SignatureCatalog::add(const clang::FunctionProtoType *Function,
                           CallbackSignature Signature) {
  const auto Existing = ByExactType.find(Function);
  if (Existing == ByExactType.end() || Existing->second.namedParameterCount() <
                                           Signature.namedParameterCount()) {
    ByExactType.insert_or_assign(Function, Signature);
    ByJsonId.insert_or_assign(pointerId(Function), Signature);
  }

  const clang::Type *Canonical = clang::ASTContext::getCanonicalType(Function);
  auto &Candidates = ByCanonicalType[Canonical];
  Candidates.push_back(std::move(Signature));
  std::ranges::stable_sort(Candidates, [](const auto &Left, const auto &Right) {
    return Left.namedParameterCount() > Right.namedParameterCount();
  });
}

} // namespace astrein
