#include "functions/flush_comment_section.hpp"

#include "functions/append_words.hpp"

#include <utility>

namespace astrein {

void flushCommentSection(CommentSection &Section, FunctionDoc &Doc) {
  if (Section.Text.empty()) {
    Section = {};
    return;
  }

  switch (Section.Type) {
  case CommentSectionKind::Brief:
    Doc.Brief = appendWords(std::move(Doc.Brief), Section.Text);
    break;
  case CommentSectionKind::Returns:
    Doc.Returns = appendWords(std::move(Doc.Returns), Section.Text);
    break;
  case CommentSectionKind::Details:
  case CommentSectionKind::None:
    Doc.Details.push_back(std::move(Section.Text));
    break;
  case CommentSectionKind::Parameter:
    if (!Section.ParameterName.empty()) {
      auto &Parameter = Doc.Parameters[Section.ParameterName];
      Parameter.Description =
          appendWords(std::move(Parameter.Description), Section.Text);
      Parameter.Direction = Section.Direction;
    }
    break;
  }
  Section = {};
}

} // namespace astrein
