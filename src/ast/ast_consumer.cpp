#include "ast/ast_consumer.hpp"

#include "ast/ast_visitor.hpp"
#include "ast/signature_catalog.hpp"
#include "functions/patch_full_ast.hpp"
#include "functions/reduced_function_json.hpp"
#include "model/run_state.hpp"

#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Mangle.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Frontend/CompilerInstance.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/raw_ostream.h"

#include <algorithm>
#include <ranges>
#include <utility>

namespace astrein {

AstConsumer::AstConsumer(llvm::raw_ostream &Output, OutputMode Mode,
                         std::string PublicHeader, RunState &State,
                         clang::CompilerInstance &Compiler,
                         const std::vector<std::string> &ApiRoots)
    : Output(Output), Mode(Mode), PublicHeader(std::move(PublicHeader)),
      State(State), Compiler(Compiler), ApiRoots(ApiRoots) {}

void AstConsumer::HandleTranslationUnit(clang::ASTContext &Context) {
  clang::PrintingPolicy Policy(Context.getLangOpts());
  Policy.SuppressScope = false;

  SignatureCatalog Signatures(Context, Policy);
  AstVisitor Visitor(Context, Signatures, ApiRoots);
  Visitor.TraverseDecl(Context.getTranslationUnitDecl());
  Signatures.finalizeDeclarations();

  if (Mode == OutputMode::Full)
    writeFull(Context, Signatures);
  else
    writeReduced(Context, Signatures, Visitor.functions(), Policy);
}

void AstConsumer::fail(llvm::StringRef Message) {
  State.Failed = true;
  const unsigned Diagnostic = Compiler.getDiagnostics().getCustomDiagID(
      clang::DiagnosticsEngine::Error, "%0");
  Compiler.getDiagnostics().Report(Diagnostic) << Message;
}

void AstConsumer::writeFull(clang::ASTContext &Context,
                            const SignatureCatalog &Signatures) {
  std::string Dump;
  llvm::raw_string_ostream DumpStream(Dump);
  Context.getTranslationUnitDecl()->dump(DumpStream, false, clang::ADOF_JSON);
  DumpStream.flush();

  llvm::Expected<llvm::json::Value> Parsed = llvm::json::parse(Dump);
  if (!Parsed) {
    fail(llvm::toString(Parsed.takeError()));
    return;
  }

  patchFullAst(*Parsed, Signatures.byJsonId(),
               Signatures.byDeclarationJsonId());
  Output << llvm::formatv("{0:2}", *Parsed) << '\n';
}

void AstConsumer::writeReduced(
    clang::ASTContext &Context, const SignatureCatalog &Signatures,
    std::vector<const clang::FunctionDecl *> Functions,
    const clang::PrintingPolicy &Policy) {
  const clang::SourceManager &Sources = Context.getSourceManager();
  std::ranges::stable_sort(Functions, [&](const clang::FunctionDecl *Left,
                                          const clang::FunctionDecl *Right) {
    return Sources.isBeforeInTranslationUnit(Left->getLocation(),
                                             Right->getLocation());
  });

  llvm::json::Object Root;
  Root["schema"] = "cpp_core_ffi_api";
  Root["schemaVersion"] = 1;
  Root["publicHeader"] = PublicHeader;

  clang::ASTNameGenerator NameGenerator(Context);
  llvm::json::Array JsonFunctions;
  JsonFunctions.reserve(Functions.size());
  for (const clang::FunctionDecl *Function : Functions)
    JsonFunctions.emplace_back(reducedFunctionJson(
        *Function, Context, Signatures, NameGenerator, Policy, ApiRoots));
  Root["functions"] = std::move(JsonFunctions);
  Output << llvm::formatv("{0:2}", llvm::json::Value(std::move(Root))) << '\n';
}

} // namespace astrein
