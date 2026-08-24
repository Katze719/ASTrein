#pragma once

#include "model/ffi_filter.hpp"
#include "model/output_mode.hpp"

#include "clang/AST/ASTConsumer.h"
#include "llvm/ADT/StringRef.h"

#include <string>
#include <vector>

namespace clang {
class ASTContext;
class CompilerInstance;
class FunctionDecl;
class PrintingPolicy;
} // namespace clang

namespace llvm {
class raw_ostream;
}

namespace astrein {

class SignatureCatalog;
struct RunState;

class AstConsumer final : public clang::ASTConsumer {
public:
  AstConsumer(llvm::raw_ostream &Output, OutputMode Mode,
              std::string PublicHeader, RunState &State,
              clang::CompilerInstance &Compiler,
              const std::vector<std::string> &ApiRoots, FfiFilter Filter);

  void HandleTranslationUnit(clang::ASTContext &Context) override;

private:
  void fail(llvm::StringRef Message);
  void writeFull(clang::ASTContext &Context,
                 const SignatureCatalog &Signatures);
  void writeReduced(clang::ASTContext &Context,
                    const SignatureCatalog &Signatures,
                    std::vector<const clang::FunctionDecl *> Functions,
                    const clang::PrintingPolicy &Policy);

  llvm::raw_ostream &Output;
  OutputMode Mode;
  std::string PublicHeader;
  RunState &State;
  clang::CompilerInstance &Compiler;
  const std::vector<std::string> &ApiRoots;
  FfiFilter Filter;
};

} // namespace astrein
