#pragma once

#include "clang/AST/RecursiveASTVisitor.h"

#include <string>
#include <unordered_set>
#include <vector>

namespace clang {
class ASTContext;
class DeclaratorDecl;
class FunctionDecl;
class SourceManager;
class TypedefNameDecl;
} // namespace clang

namespace astrein {

class SignatureCatalog;

class AstVisitor : public clang::RecursiveASTVisitor<AstVisitor> {
public:
  AstVisitor(clang::ASTContext &Context, SignatureCatalog &Signatures,
             const std::vector<std::string> &ApiRoots);

  bool VisitDeclaratorDecl(clang::DeclaratorDecl *Declaration);
  bool VisitTypedefNameDecl(clang::TypedefNameDecl *Declaration);
  bool VisitFunctionDecl(clang::FunctionDecl *Declaration);

  [[nodiscard]] const std::vector<const clang::FunctionDecl *> &
  functions() const;

private:
  [[nodiscard]] bool
  isFfiFunction(const clang::FunctionDecl &Declaration) const;

  clang::SourceManager &Sources;
  SignatureCatalog &Signatures;
  const std::vector<std::string> &ApiRoots;
  std::unordered_set<const clang::FunctionDecl *> SeenFunctions;
  std::vector<const clang::FunctionDecl *> Functions;
};

} // namespace astrein
