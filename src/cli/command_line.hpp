#pragma once

#include "cli/command_line_action.hpp"
#include "model/ffi_filter.hpp"
#include "model/output_mode.hpp"

#include <optional>
#include <string>
#include <vector>

namespace astrein {

struct CommandLine {
  CommandLineAction Action = CommandLineAction::Run;
  OutputMode Mode = OutputMode::Full;
  std::string OutputPath = "-";
  std::string PublicHeader;
  std::vector<std::string> ApiRoots;
  FfiFilter Filter;
  std::string SourcePath;
  std::optional<std::string> CompilationDatabasePath;
  std::vector<std::string> ClangArguments;
};

} // namespace astrein
