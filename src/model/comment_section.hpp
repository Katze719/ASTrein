#pragma once

#include "model/comment_section_kind.hpp"

#include <string>

namespace astrein {

struct CommentSection {
  CommentSectionKind Type = CommentSectionKind::None;
  std::string ParameterName;
  std::string Direction = "in";
  std::string Text;
};

} // namespace astrein
