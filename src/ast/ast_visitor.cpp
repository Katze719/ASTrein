#include "ast/ast_visitor.hpp"

#include "ast/signature_catalog.hpp"
#include "functions/path_is_below.hpp"

#include "clang/AST/ASTContext.h"
#include "clang/AST/Attr.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "clang/Basic/SourceManager.h"

#include <algorithm>
#include <ranges>

namespace astrein {

AstVisitor::AstVisitor(clang::ASTContext &Context, SignatureCatalog &Signatures,
                       const std::vector<std::string> &ApiRoots,
                       FfiFilter Filter)
    : Sources(Context.getSourceManager()), Signatures(Signatures),
      ApiRoots(ApiRoots), Filter(Filter) {}

bool AstVisitor::VisitDeclaratorDecl(clang::DeclaratorDecl *Declaration) {
  Signatures.collect(Declaration);
  return true;
}

bool AstVisitor::VisitTypedefNameDecl(clang::TypedefNameDecl *Declaration) {
  Signatures.collect(Declaration);
  return true;
}

bool AstVisitor::VisitFunctionDecl(clang::FunctionDecl *Declaration) {
  if (isFfiFunction(*Declaration)) {
    const clang::FunctionDecl *Canonical = Declaration->getCanonicalDecl();
    if (SeenFunctions.insert(Canonical).second)
      Functions.push_back(Declaration);
  }
  return true;
}

const std::vector<const clang::FunctionDecl *> &AstVisitor::functions() const {
  return Functions;
}

bool AstVisitor::isFfiFunction(const clang::FunctionDecl &Declaration) const {
  if (Declaration.isImplicit() || Declaration.getLocation().isInvalid() ||
      llvm::isa<clang::CXXMethodDecl>(&Declaration) ||
      Declaration.getTemplatedKind() != clang::FunctionDecl::TK_NonTemplate ||
      Declaration.getIdentifier() == nullptr ||
      !Declaration.hasExternalFormalLinkage())
    return false;

  if (Filter.RequireCLinkage && !Declaration.isExternC())
    return false;

  if (Filter.RequireDefaultVisibility) {
    const auto *Visibility = Declaration.getAttr<clang::VisibilityAttr>();
    const bool HasDefaultVisibility =
        Visibility != nullptr &&
        Visibility->getVisibility() == clang::VisibilityAttr::Default;
    if (!HasDefaultVisibility && !Declaration.hasAttr<clang::DLLExportAttr>())
      return false;
  }

  const clang::SourceLocation Location =
      Sources.getExpansionLoc(Declaration.getLocation());
  if (Sources.isInSystemHeader(Location))
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

} // namespace astrein
