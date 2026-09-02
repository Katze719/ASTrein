#include "ast/ast_consumer.hpp"

#include "ast/ast_visitor.hpp"
#include "ast/signature_catalog.hpp"
#include "functions/patch_full_ast.hpp"
#include "functions/reduced_enum_json.hpp"
#include "functions/reduced_function_json.hpp"
#include "functions/reduced_struct_json.hpp"
#include "functions/used_types.hpp"
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
namespace {

void writeJson(llvm::raw_ostream &Output, const llvm::json::Value &Value,
               bool Minify) {
  if (Minify)
    Output << llvm::formatv("{0}", Value) << '\n';
  else
    Output << llvm::formatv("{0:2}", Value) << '\n';
}

} // namespace

AstConsumer::AstConsumer(llvm::raw_ostream &Output, OutputMode Mode,
                         bool Minify, std::string PublicHeader, RunState &State,
                         clang::CompilerInstance &Compiler,
                         const std::vector<std::string> &ApiRoots,
                         FfiFilter Filter)
    : Output(Output), Mode(Mode), Minify(Minify),
      PublicHeader(std::move(PublicHeader)), State(State), Compiler(Compiler),
      ApiRoots(ApiRoots), Filter(Filter) {}

void AstConsumer::HandleTranslationUnit(clang::ASTContext &Context) {
  clang::PrintingPolicy Policy(Context.getLangOpts());
  Policy.SuppressScope = false;

  SignatureCatalog Signatures(Context, Policy);
  AstVisitor Visitor(Context, Signatures, ApiRoots, Filter);
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
  writeJson(Output, *Parsed, Minify);
}

void AstConsumer::writeReduced(
    clang::ASTContext &Context, const SignatureCatalog &Signatures,
    std::vector<const clang::FunctionDecl *> Functions,
    const clang::PrintingPolicy &Policy) {
  std::ranges::stable_sort(Functions, [](const clang::FunctionDecl *Left,
                                         const clang::FunctionDecl *Right) {
    return Left->getQualifiedNameAsString() < Right->getQualifiedNameAsString();
  });

  llvm::json::Object Root;
  Root["$schema"] =
      "https://raw.githubusercontent.com/Katze719/ASTrein/main/schema/"
      "astrein-ffi-api-v3.schema.json";
  Root["schema"] = "astrein_ffi_api";
  Root["schemaVersion"] = 3;
  Root["publicHeader"] = PublicHeader;

  const UsedTypes Types = usedTypes(Functions, Context, ApiRoots);

  llvm::json::Array JsonEnums;
  JsonEnums.reserve(Types.Enums.size());
  for (const EnumDefinition &Definition : Types.Enums)
    JsonEnums.emplace_back(
        reducedEnumJson(Definition, Context, Policy, ApiRoots));
  Root["enums"] = std::move(JsonEnums);

  clang::ASTNameGenerator NameGenerator(Context);
  llvm::json::Array JsonFunctions;
  JsonFunctions.reserve(Functions.size());
  for (const clang::FunctionDecl *Function : Functions)
    JsonFunctions.emplace_back(reducedFunctionJson(
        *Function, Context, Signatures, NameGenerator, Policy, ApiRoots));
  Root["functions"] = std::move(JsonFunctions);

  llvm::json::Array JsonStructs;
  JsonStructs.reserve(Types.Structs.size());
  for (const StructDefinition &Definition : Types.Structs)
    JsonStructs.emplace_back(
        reducedStructJson(Definition, Context, Policy, ApiRoots));
  Root["structs"] = std::move(JsonStructs);
  writeJson(Output, llvm::json::Value(std::move(Root)), Minify);
}

} // namespace astrein
