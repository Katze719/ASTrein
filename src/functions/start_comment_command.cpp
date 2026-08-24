#include "functions/start_comment_command.hpp"

#include "model/comment_section.hpp"

namespace astrein {

bool startCommentCommand(llvm::StringRef Line, CommentSection &Section) {
  Line = Line.trim();
  if (Line.empty() || (Line.front() != '@' && Line.front() != '\\'))
    return false;
  Line = Line.drop_front();

  const std::size_t CommandEnd = Line.find_first_of("[ \t");
  const llvm::StringRef Command = Line.take_front(CommandEnd);
  Line = CommandEnd == llvm::StringRef::npos ? llvm::StringRef{}
                                             : Line.drop_front(CommandEnd);

  if (Command == "brief") {
    Section.Type = CommentSectionKind::Brief;
  } else if (Command == "return" || Command == "returns" ||
             Command == "result") {
    Section.Type = CommentSectionKind::Returns;
  } else if (Command == "details" || Command == "remark" ||
             Command == "remarks" || Command == "note") {
    Section.Type = CommentSectionKind::Details;
  } else if (Command == "param") {
    Section.Type = CommentSectionKind::Parameter;
    Line = Line.ltrim();
    if (Line.starts_with("[")) {
      const std::size_t Close = Line.find(']');
      if (Close != llvm::StringRef::npos) {
        Section.Direction = Line.slice(1, Close).trim().str();
        Line = Line.drop_front(Close + 1).ltrim();
      }
    }
    const std::size_t NameEnd = Line.find_first_of(" \t");
    Section.ParameterName = Line.take_front(NameEnd).str();
    Line = NameEnd == llvm::StringRef::npos ? llvm::StringRef{}
                                            : Line.drop_front(NameEnd);
  } else {
    return false;
  }

  Section.Text = Line.trim().str();
  return true;
}

} // namespace astrein
