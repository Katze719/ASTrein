#include "functions/used_types.hpp"

#include "functions/path_is_below.hpp"

#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/Type.h"
#include "clang/Basic/SourceManager.h"

#include <algorithm>
#include <ranges>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace astrein {
namespace {

class UsedTypeCollector {
public:
  UsedTypeCollector(clang::ASTContext &Context,
                    const std::vector<std::string> &ApiRoots)
      : Context(Context), Sources(Context.getSourceManager()),
        ApiRoots(ApiRoots) {}

  UsedTypes collect(const std::vector<const clang::FunctionDecl *> &Functions) {
    for (const clang::FunctionDecl *Function : Functions) {
      collectType(Function->getReturnType());
      for (const clang::ParmVarDecl *Parameter : Function->parameters())
        collectType(Parameter->getType());
    }

    UsedTypes Result;
    Result.Structs.reserve(Records.size());
    for (const auto &[Canonical, Declaration] : Records) {
      StructDefinition Definition;
      Definition.Declaration = Declaration;
      Definition.Name = tagName(*Canonical);
      if (const auto Found = RecordAliases.find(Canonical);
          Found != RecordAliases.end()) {
        Definition.Aliases.assign(Found->second.begin(), Found->second.end());
        std::ranges::sort(Definition.Aliases);
        std::erase(Definition.Aliases, Definition.Name);
      }
      Result.Structs.push_back(std::move(Definition));
    }

    std::ranges::stable_sort(Result.Structs, [](const StructDefinition &Left,
                                                const StructDefinition &Right) {
      return Left.Name < Right.Name;
    });

    Result.Enums.reserve(Enums.size());
    for (const auto &[Canonical, Declaration] : Enums) {
      EnumDefinition Definition;
      Definition.Declaration = Declaration;
      Definition.Name = tagName(*Canonical);
      if (const auto Found = EnumAliases.find(Canonical);
          Found != EnumAliases.end()) {
        Definition.Aliases.assign(Found->second.begin(), Found->second.end());
        std::ranges::sort(Definition.Aliases);
        std::erase(Definition.Aliases, Definition.Name);
      }
      Result.Enums.push_back(std::move(Definition));
    }

    std::ranges::stable_sort(Result.Enums, [](const EnumDefinition &Left,
                                              const EnumDefinition &Right) {
      return Left.Name < Right.Name;
    });
    return Result;
  }

private:
  [[nodiscard]] bool isApiDeclaration(const clang::Decl &Declaration) const {
    const clang::SourceLocation Location =
        Sources.getExpansionLoc(Declaration.getLocation());
    if (Location.isInvalid() || Sources.isInSystemHeader(Location))
      return false;
    if (ApiRoots.empty())
      return true;

    const clang::PresumedLoc Presumed = Sources.getPresumedLoc(Location);
    if (Presumed.isInvalid())
      return false;
    const llvm::StringRef Filename(Presumed.getFilename());
    return std::ranges::any_of(ApiRoots, [&](const std::string &Root) {
      return pathIsBelow(Filename, Root);
    });
  }

  [[nodiscard]] static std::string tagName(const clang::TagDecl &Declaration) {
    if (!Declaration.getName().empty())
      return Declaration.getQualifiedNameAsString();
    if (const clang::TypedefNameDecl *Typedef =
            Declaration.getTypedefNameForAnonDecl())
      return Typedef->getQualifiedNameAsString();
    return {};
  }

  void rememberAlias(clang::QualType Type) {
    const auto *Typedef =
        llvm::dyn_cast<clang::TypedefType>(Type.getTypePtrOrNull());
    if (Typedef == nullptr)
      return;

    const clang::QualType Underlying =
        Typedef->getDecl()->getUnderlyingType().getCanonicalType();
    const std::string Alias = Typedef->getDecl()->getQualifiedNameAsString();
    if (const auto *RecordType = Underlying->getAs<clang::RecordType>();
        RecordType != nullptr && RecordType->getDecl()->isStruct()) {
      const auto *Canonical = llvm::cast<clang::RecordDecl>(
          RecordType->getDecl()->getCanonicalDecl());
      RecordAliases[Canonical].insert(Alias);
      return;
    }

    if (const auto *EnumType = Underlying->getAs<clang::EnumType>()) {
      const auto *Canonical =
          llvm::cast<clang::EnumDecl>(EnumType->getDecl()->getCanonicalDecl());
      EnumAliases[Canonical].insert(Alias);
    }
  }

  void collectRecord(const clang::RecordDecl &Record) {
    if (!Record.isStruct())
      return;

    const clang::RecordDecl *Canonical =
        llvm::cast<clang::RecordDecl>(Record.getCanonicalDecl());
    if (Records.contains(Canonical) || RejectedRecords.contains(Canonical))
      return;

    const clang::RecordDecl *Selected = nullptr;
    if (const clang::RecordDecl *Definition = Canonical->getDefinition();
        Definition != nullptr && isApiDeclaration(*Definition))
      Selected = Definition;
    if (Selected == nullptr) {
      for (const clang::TagDecl *TagRedeclaration : Canonical->redecls()) {
        const auto *Redeclaration =
            llvm::cast<clang::RecordDecl>(TagRedeclaration);
        if (isApiDeclaration(*Redeclaration)) {
          Selected = Redeclaration;
          break;
        }
      }
    }

    if (Selected == nullptr || tagName(*Canonical).empty()) {
      RejectedRecords.insert(Canonical);
      return;
    }

    Records.emplace(Canonical, Selected);
    if (!Selected->isCompleteDefinition())
      return;
    for (const clang::FieldDecl *Field : Selected->fields())
      collectType(Field->getType());
  }

  void collectEnum(const clang::EnumDecl &Enum) {
    const auto *Canonical =
        llvm::cast<clang::EnumDecl>(Enum.getCanonicalDecl());
    if (Enums.contains(Canonical) || RejectedEnums.contains(Canonical))
      return;

    const clang::EnumDecl *Selected = nullptr;
    if (const clang::EnumDecl *Definition = Canonical->getDefinition();
        Definition != nullptr && isApiDeclaration(*Definition))
      Selected = Definition;
    if (Selected == nullptr) {
      for (const clang::TagDecl *TagRedeclaration : Canonical->redecls()) {
        const auto *Redeclaration =
            llvm::cast<clang::EnumDecl>(TagRedeclaration);
        if (isApiDeclaration(*Redeclaration)) {
          Selected = Redeclaration;
          break;
        }
      }
    }

    if (Selected == nullptr || tagName(*Canonical).empty()) {
      RejectedEnums.insert(Canonical);
      return;
    }

    Enums.emplace(Canonical, Selected);
  }

  void collectType(clang::QualType Type) {
    if (Type.isNull())
      return;

    rememberAlias(Type);
    const clang::Type *Raw = Type.getTypePtr();

    if (const auto *Typedef = llvm::dyn_cast<clang::TypedefType>(Raw)) {
      collectType(Typedef->getDecl()->getUnderlyingType());
      return;
    }

    if (const auto *Pointer = llvm::dyn_cast<clang::PointerType>(Raw)) {
      collectType(Pointer->getPointeeType());
      return;
    }
    if (const auto *Reference = llvm::dyn_cast<clang::ReferenceType>(Raw)) {
      collectType(Reference->getPointeeType());
      return;
    }
    if (const auto *Array = llvm::dyn_cast<clang::ArrayType>(Raw)) {
      collectType(Array->getElementType());
      return;
    }
    if (const auto *Function = llvm::dyn_cast<clang::FunctionProtoType>(Raw)) {
      collectType(Function->getReturnType());
      for (clang::QualType Parameter : Function->getParamTypes())
        collectType(Parameter);
      return;
    }
    if (const auto *Function =
            llvm::dyn_cast<clang::FunctionNoProtoType>(Raw)) {
      collectType(Function->getReturnType());
      return;
    }
    if (const auto *MemberPointer =
            llvm::dyn_cast<clang::MemberPointerType>(Raw)) {
      collectType(MemberPointer->getPointeeType());
      if (const clang::CXXRecordDecl *Class =
              MemberPointer->getMostRecentCXXRecordDecl())
        collectRecord(*Class);
      return;
    }
    if (const auto *BlockPointer =
            llvm::dyn_cast<clang::BlockPointerType>(Raw)) {
      collectType(BlockPointer->getPointeeType());
      return;
    }
    if (const auto *Atomic = llvm::dyn_cast<clang::AtomicType>(Raw)) {
      collectType(Atomic->getValueType());
      return;
    }
    if (const auto *RecordType = llvm::dyn_cast<clang::RecordType>(Raw))
      collectRecord(*RecordType->getDecl());
    if (const auto *EnumType = llvm::dyn_cast<clang::EnumType>(Raw))
      collectEnum(*EnumType->getDecl());

    const clang::QualType Desugared = Type.getSingleStepDesugaredType(Context);
    if (Desugared != Type)
      collectType(Desugared);
  }

  clang::ASTContext &Context;
  clang::SourceManager &Sources;
  const std::vector<std::string> &ApiRoots;
  std::unordered_map<const clang::RecordDecl *, const clang::RecordDecl *>
      Records;
  std::unordered_map<const clang::RecordDecl *, std::unordered_set<std::string>>
      RecordAliases;
  std::unordered_map<const clang::EnumDecl *, const clang::EnumDecl *> Enums;
  std::unordered_map<const clang::EnumDecl *, std::unordered_set<std::string>>
      EnumAliases;
  std::unordered_set<const clang::RecordDecl *> RejectedRecords;
  std::unordered_set<const clang::EnumDecl *> RejectedEnums;
};

} // namespace

UsedTypes usedTypes(const std::vector<const clang::FunctionDecl *> &Functions,
                    clang::ASTContext &Context,
                    const std::vector<std::string> &ApiRoots) {
  return UsedTypeCollector(Context, ApiRoots).collect(Functions);
}

} // namespace astrein
