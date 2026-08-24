#include "cli/argument_parser.hpp"

#include "clang/Basic/Version.h"

#include <string_view>

namespace astrein {

std::expected<CommandLine, std::string>
ArgumentParser::parse(int argc, const char *const *argv) const {
  CommandLine Result;
  bool ParseClangArguments = false;

  for (int Index = 1; Index < argc; ++Index) {
    const std::string_view Argument(argv[Index]);

    if (ParseClangArguments) {
      Result.ClangArguments.emplace_back(Argument);
      continue;
    }

    if (Argument == "--") {
      ParseClangArguments = true;
      continue;
    }
    if (Argument == "-h" || Argument == "--help") {
      Result.Action = CommandLineAction::Help;
      return Result;
    }
    if (Argument == "-V" || Argument == "--version") {
      Result.Action = CommandLineAction::Version;
      return Result;
    }

    if (Argument == "--mode") {
      if (++Index >= argc)
        return std::unexpected("option '--mode' requires a value");
      const std::string_view Value(argv[Index]);
      if (Value == "full")
        Result.Mode = OutputMode::Full;
      else if (Value == "reduced")
        Result.Mode = OutputMode::Reduced;
      else
        return std::unexpected("invalid value for '--mode': '" +
                               std::string(Value) +
                               "' (expected 'full' or 'reduced')");
      continue;
    }
    if (Argument.starts_with("--mode=")) {
      const std::string_view Value = Argument.substr(7);
      if (Value == "full")
        Result.Mode = OutputMode::Full;
      else if (Value == "reduced")
        Result.Mode = OutputMode::Reduced;
      else
        return std::unexpected("invalid value for '--mode': '" +
                               std::string(Value) +
                               "' (expected 'full' or 'reduced')");
      continue;
    }

    if (Argument == "-o" || Argument == "--output") {
      if (++Index >= argc)
        return std::unexpected("option '" + std::string(Argument) +
                               "' requires a value");
      Result.OutputPath = argv[Index];
      continue;
    }
    if (Argument.starts_with("--output=")) {
      Result.OutputPath = Argument.substr(9);
      if (Result.OutputPath.empty())
        return std::unexpected("option '--output' requires a value");
      continue;
    }

    if (Argument == "--public-header") {
      if (++Index >= argc)
        return std::unexpected("option '--public-header' requires a value");
      Result.PublicHeader = argv[Index];
      continue;
    }
    if (Argument.starts_with("--public-header=")) {
      Result.PublicHeader = Argument.substr(16);
      if (Result.PublicHeader.empty())
        return std::unexpected("option '--public-header' requires a value");
      continue;
    }

    if (Argument == "--api-root") {
      if (++Index >= argc)
        return std::unexpected("option '--api-root' requires a value");
      Result.ApiRoots.emplace_back(argv[Index]);
      continue;
    }
    if (Argument.starts_with("--api-root=")) {
      const std::string_view Value = Argument.substr(11);
      if (Value.empty())
        return std::unexpected("option '--api-root' requires a value");
      Result.ApiRoots.emplace_back(Value);
      continue;
    }

    if (Argument == "--require-c-linkage") {
      Result.Filter.RequireCLinkage = true;
      continue;
    }
    if (Argument == "--require-default-visibility") {
      Result.Filter.RequireDefaultVisibility = true;
      continue;
    }

    if (Argument == "-p" || Argument == "--build-path") {
      if (++Index >= argc)
        return std::unexpected("option '" + std::string(Argument) +
                               "' requires a value");
      Result.BuildPath = argv[Index];
      continue;
    }
    if (Argument.starts_with("-p=")) {
      const std::string_view Value = Argument.substr(3);
      if (Value.empty())
        return std::unexpected("option '-p' requires a value");
      Result.BuildPath = std::string(Value);
      continue;
    }
    if (Argument.starts_with("--build-path=")) {
      const std::string_view Value = Argument.substr(13);
      if (Value.empty())
        return std::unexpected("option '--build-path' requires a value");
      Result.BuildPath = std::string(Value);
      continue;
    }

    if (Argument.starts_with('-'))
      return std::unexpected("unknown option '" + std::string(Argument) + "'");

    if (!Result.SourcePath.empty())
      return std::unexpected("unexpected positional argument '" +
                             std::string(Argument) +
                             "' (exactly one translation unit is required)");
    Result.SourcePath = Argument;
  }

  if (Result.SourcePath.empty())
    return std::unexpected("missing translation unit");
  return Result;
}

std::string ArgumentParser::helpText() const {
  return R"(Usage: astrein [options] <translation-unit> [-- <clang-arguments>...]

Emit Clang's JSON AST or a reduced JSON API for FFI binding generators.

Options:
  -h, --help                 Show this help text and exit
  -V, --version              Show ASTrein and Clang versions and exit
      --mode <mode>          Output mode: full (default) or reduced
  -o, --output <path>        Write JSON to path; use '-' for stdout (default)
      --public-header <path> Header spelling stored in reduced output
      --api-root <directory> Include declarations below this directory;
                             may be specified more than once
      --require-c-linkage    Include only functions with C language linkage
      --require-default-visibility
                             Include only functions explicitly marked with
                             default visibility
  -p, --build-path <dir>     Load compile_commands.json from this directory

Arguments after '--' are passed to Clang. If -p is also present, they are
appended to the matching compilation database command.

Examples:
  astrein --output ast.json include/api.hpp -- -std=c++26 -Iinclude
  astrein --mode=reduced --api-root=include --require-c-linkage \
    --require-default-visibility include/api.hpp -- -std=c++26
  astrein -p build include/api.hpp
)";
}

std::string ArgumentParser::versionText() const {
  return "ASTrein " ASTREIN_VERSION "\nClang " CLANG_VERSION_STRING "\n";
}

} // namespace astrein
