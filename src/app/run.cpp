#include "app/run.hpp"

#include "ast/ast_action_factory.hpp"
#include "cli/argument_parser.hpp"
#include "functions/api_relative_path.hpp"
#include "functions/strip_gcc_module_arguments.hpp"
#include "model/run_state.hpp"

#include "clang/Tooling/ArgumentsAdjusters.h"
#include "clang/Tooling/CompilationDatabase.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/TargetParser/Host.h"

#include <memory>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace astrein {

int run(int argc, const char *const *argv) {
  const ArgumentParser Parser;
  auto Parsed = Parser.parse(argc, argv);
  if (!Parsed) {
    llvm::errs() << "astrein: error: " << Parsed.error() << "\n\n"
                 << Parser.helpText();
    return 2;
  }

  CommandLine Options = std::move(*Parsed);
  if (Options.Action == CommandLineAction::Help) {
    llvm::outs() << Parser.helpText();
    return 0;
  }
  if (Options.Action == CommandLineAction::Version) {
    llvm::outs() << Parser.versionText();
    return 0;
  }

  for (std::string &Root : Options.ApiRoots) {
    llvm::SmallString<256> Absolute(Root);
    if (std::error_code Error = llvm::sys::fs::make_absolute(Absolute)) {
      llvm::errs() << "astrein: error: cannot resolve API root '" << Root
                   << "': " << Error.message() << '\n';
      return 2;
    }
    llvm::sys::path::remove_dots(Absolute, true);
    Root = Absolute.str().str();
  }

  std::unique_ptr<clang::tooling::CompilationDatabase> Compilations;
  if (Options.BuildPath.has_value()) {
    std::string Error;
    Compilations = clang::tooling::CompilationDatabase::loadFromDirectory(
        *Options.BuildPath, Error);
    if (!Compilations) {
      llvm::errs() << "astrein: error: cannot load compilation database from '"
                   << *Options.BuildPath << "': " << Error << '\n';
      return 2;
    }
  } else {
    Compilations = std::make_unique<clang::tooling::FixedCompilationDatabase>(
        ".", Options.ClangArguments);
  }

  std::unique_ptr<llvm::raw_fd_ostream> FileOutput;
  llvm::raw_ostream *Output = &llvm::outs();
  if (Options.OutputPath != "-") {
    std::error_code Error;
    FileOutput = std::make_unique<llvm::raw_fd_ostream>(
        Options.OutputPath, Error, llvm::sys::fs::OF_Text);
    if (Error) {
      llvm::errs() << "astrein: error: cannot open '" << Options.OutputPath
                   << "': " << Error.message() << '\n';
      return 2;
    }
    Output = FileOutput.get();
  }

  std::string Header = Options.PublicHeader;
  if (Header.empty())
    Header = apiRelativePath(Options.SourcePath, Options.ApiRoots);

  const std::vector<std::string> Sources{Options.SourcePath};
  RunState State;
  clang::tooling::ClangTool Tool(*Compilations, Sources);
  Tool.appendArgumentsAdjuster(stripGccModuleArguments());
  Tool.appendArgumentsAdjuster(
      clang::tooling::getClangStripDependencyFileAdjuster());
  if (llvm::sys::getDefaultTargetTriple().empty()) {
    clang::tooling::CommandLineArguments HostTarget{
        std::string("--target=") + llvm::sys::getProcessTriple()};
    Tool.appendArgumentsAdjuster(clang::tooling::getInsertArgumentAdjuster(
        HostTarget, clang::tooling::ArgumentInsertPosition::BEGIN));
  }
  if (Options.BuildPath.has_value() && !Options.ClangArguments.empty())
    Tool.appendArgumentsAdjuster(clang::tooling::getInsertArgumentAdjuster(
        Options.ClangArguments, clang::tooling::ArgumentInsertPosition::END));
  Tool.appendArgumentsAdjuster(clang::tooling::getInsertArgumentAdjuster(
      "-fparse-all-comments", clang::tooling::ArgumentInsertPosition::END));

  AstActionFactory Factory(*Output, Options.Mode, std::move(Header), State,
                           Options.ApiRoots, Options.Filter);
  const int Result = Tool.run(&Factory);
  Output->flush();
  return Result == 0 && !State.Failed ? 0 : 1;
}

} // namespace astrein
