#include "functions/extract_documentation.hpp"

#include "functions/append_words.hpp"
#include "functions/flush_comment_section.hpp"
#include "functions/start_comment_command.hpp"
#include "model/comment_section.hpp"

#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/RawCommentList.h"
#include "llvm/ADT/StringRef.h"

#include <algorithm>
#include <utility>

namespace astrein {

FunctionDoc extractDocumentation(const clang::FunctionDecl &Declaration,
                                 clang::ASTContext &Context) {
  FunctionDoc Result;
  const clang::RawComment *Raw =
      Context.getRawCommentForAnyRedecl(&Declaration);
  if (Raw == nullptr)
    return Result;

  const std::string Formatted = Raw->getFormattedText(
      Context.getSourceManager(), Context.getDiagnostics());
  CommentSection Current;

  llvm::StringRef Remaining(Formatted);
  while (!Remaining.empty()) {
    auto [Line, Rest] = Remaining.split('\n');
    Remaining = Rest;
    Line = Line.trim();

    if (Line.empty()) {
      flushCommentSection(Current, Result);
      continue;
    }

    CommentSection Next;
    if (startCommentCommand(Line, Next)) {
      flushCommentSection(Current, Result);
      Current = std::move(Next);
    } else {
      if (Current.Type == CommentSectionKind::None)
        Current.Type = CommentSectionKind::Details;
      Current.Text = appendWords(std::move(Current.Text), Line);
    }
  }
  flushCommentSection(Current, Result);

  if (Result.Brief.empty())
    Result.Brief = Raw->getBriefText(Context);

  std::erase_if(Result.Details, [&](const std::string &Detail) {
    return Detail.empty() || Detail == Result.Brief;
  });
  return Result;
}

} // namespace astrein
