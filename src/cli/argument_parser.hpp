#pragma once

#include "cli/command_line.hpp"

#include <expected>
#include <string>

namespace astrein {

class ArgumentParser {
public:
  [[nodiscard]] std::expected<CommandLine, std::string>
  parse(int argc, const char *const *argv) const;

  [[nodiscard]] std::string helpText() const;
  [[nodiscard]] std::string versionText() const;
};

} // namespace astrein
