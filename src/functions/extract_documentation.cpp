#include "functions/extract_documentation.hpp"

#include "functions/append_words.hpp"

#include "clang/AST/ASTContext.h"
#include "clang/AST/Comment.h"
#include "clang/AST/Decl.h"
#include "clang/AST/RawCommentList.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Casting.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <utility>

namespace astrein {
namespace {

bool closesWord(char Character) {
  return llvm::StringRef(".,;!?)]}").contains(Character);
}

bool opensWord(char Character) {
  return llvm::StringRef("([{\"").contains(Character);
}

std::string normalizeText(llvm::StringRef Text) {
  std::string Result;
  Result.reserve(Text.size());
  bool PendingSpace = false;

  for (const char Character : Text) {
    if (std::isspace(static_cast<unsigned char>(Character)) != 0) {
      PendingSpace = !Result.empty();
      continue;
    }

    if (PendingSpace && !Result.empty() && !closesWord(Character) &&
        !opensWord(Result.back()))
      Result.push_back(' ');
    Result.push_back(Character);
    PendingSpace = false;
  }
  return Result;
}

std::string inlineArgument(llvm::StringRef Argument, llvm::StringRef Prefix,
                           llvm::StringRef Suffix) {
  Argument = Argument.trim();
  llvm::StringRef Punctuation;
  if (!Argument.empty() && llvm::StringRef(".,;:").contains(Argument.back())) {
    Punctuation = Argument.take_back();
    Argument = Argument.drop_back();
  }
  return (Prefix + Argument + Suffix + Punctuation).str();
}

void appendCommentText(const clang::comments::Comment &Comment,
                       std::string &Output) {
  if (const auto *Text =
          llvm::dyn_cast<clang::comments::TextComment>(&Comment)) {
    Output.append(Text->getText());
    return;
  }

  if (const auto *Command =
          llvm::dyn_cast<clang::comments::InlineCommandComment>(&Comment)) {
    for (unsigned Index = 0; Index != Command->getNumArgs(); ++Index) {
      if (!Output.empty())
        Output.push_back(' ');
      const llvm::StringRef Argument = Command->getArgText(Index);
      switch (Command->getRenderKind()) {
      case clang::comments::InlineCommandRenderKind::Bold:
        Output += inlineArgument(Argument, "**", "**");
        break;
      case clang::comments::InlineCommandRenderKind::Monospaced:
        Output += inlineArgument(Argument, "`", "`");
        break;
      case clang::comments::InlineCommandRenderKind::Emphasized:
        Output += inlineArgument(Argument, "*", "*");
        break;
      case clang::comments::InlineCommandRenderKind::Normal:
      case clang::comments::InlineCommandRenderKind::Anchor:
        Output.append(Argument);
        break;
      }
    }
    return;
  }

  for (auto Iterator = Comment.child_begin(); Iterator != Comment.child_end();
       ++Iterator) {
    if (*Iterator == nullptr)
      continue;
    std::string ChildText;
    appendCommentText(**Iterator, ChildText);
    if (ChildText.empty())
      continue;
    if (!Output.empty())
      Output.push_back(' ');
    Output += ChildText;
  }
}

std::string commentText(const clang::comments::Comment &Comment) {
  std::string Result;
  appendCommentText(Comment, Result);
  return normalizeText(Result);
}

std::string
parameterDirection(clang::comments::ParamCommandPassDirection Direction) {
  switch (Direction) {
  case clang::comments::ParamCommandPassDirection::In:
    return "in";
  case clang::comments::ParamCommandPassDirection::Out:
    return "out";
  case clang::comments::ParamCommandPassDirection::InOut:
    return "in,out";
  }
  return "in";
}

std::string verbatimText(const clang::comments::VerbatimBlockComment &Block,
                         const clang::comments::CommandTraits &Traits) {
  std::string Result = "```";
  unsigned FirstLine = 0;
  if (Block.getNumArgs() != 0) {
    llvm::StringRef Language = Block.getArgText(0).trim();
    if (Language.starts_with("{.") && Language.ends_with("}"))
      Language = Language.drop_front(2).drop_back();
    Result.append(Language);
  } else if (Block.getNumLines() != 0) {
    llvm::StringRef Language = Block.getText(0).trim();
    if (Language.starts_with("{.") && Language.ends_with("}")) {
      Result.append(Language.drop_front(2).drop_back());
      FirstLine = 1;
    } else if (Block.getCommandName(Traits) != "code") {
      Result.append(Block.getCommandName(Traits));
    }
  } else if (Block.getCommandName(Traits) != "code") {
    Result.append(Block.getCommandName(Traits));
  }
  Result.push_back('\n');

  std::size_t CommonIndent = std::string::npos;
  for (unsigned Index = FirstLine; Index != Block.getNumLines(); ++Index) {
    const llvm::StringRef Line = Block.getText(Index);
    if (Line.trim().empty())
      continue;
    CommonIndent = std::min(CommonIndent, Line.size() - Line.ltrim().size());
  }
  if (CommonIndent == std::string::npos)
    CommonIndent = 0;

  for (unsigned Index = FirstLine; Index != Block.getNumLines(); ++Index) {
    Result.append(Block.getText(Index).drop_front(CommonIndent));
    Result.push_back('\n');
  }
  Result += "```";
  return Result;
}

} // namespace

FunctionDoc extractDocumentation(const clang::FunctionDecl &Declaration,
                                 clang::ASTContext &Context) {
  FunctionDoc Result;
  const clang::comments::FullComment *Full =
      Context.getCommentForDecl(&Declaration, nullptr);
  if (Full == nullptr)
    return Result;

  const clang::comments::CommandTraits &Traits =
      Context.getCommentCommandTraits();
  for (auto Iterator = Full->child_begin(); Iterator != Full->child_end();
       ++Iterator) {
    const clang::comments::Comment *Child = *Iterator;
    if (Child == nullptr)
      continue;

    if (const auto *Parameter =
            llvm::dyn_cast<clang::comments::ParamCommandComment>(Child)) {
      if (!Parameter->hasParamName())
        continue;
      const std::string Description = commentText(*Parameter);
      if (Description.empty())
        continue;
      auto &Doc = Result.Parameters[Parameter->getParamNameAsWritten().str()];
      Doc.Description = appendWords(std::move(Doc.Description), Description);
      Doc.Direction = parameterDirection(Parameter->getDirection());
      continue;
    }

    if (const auto *Verbatim =
            llvm::dyn_cast<clang::comments::VerbatimBlockComment>(Child)) {
      Result.Details.push_back(verbatimText(*Verbatim, Traits));
      continue;
    }

    if (const auto *Command =
            llvm::dyn_cast<clang::comments::BlockCommandComment>(Child)) {
      const llvm::StringRef Name = Command->getCommandName(Traits);
      const std::string Text = commentText(*Command);
      if (Text.empty())
        continue;
      if (Name == "brief")
        Result.Brief = appendWords(std::move(Result.Brief), Text);
      else if (Name == "return" || Name == "returns" || Name == "result")
        Result.Returns = appendWords(std::move(Result.Returns), Text);
      else if (Name == "details" || Name == "note" || Name == "remark" ||
               Name == "remarks")
        Result.Details.push_back(Text);
      continue;
    }

    if (llvm::isa<clang::comments::ParagraphComment>(Child)) {
      const std::string Text = commentText(*Child);
      if (!Text.empty())
        Result.Details.push_back(Text);
    }
  }

  if (Result.Brief.empty()) {
    if (const clang::RawComment *Raw =
            Context.getRawCommentForAnyRedecl(&Declaration))
      Result.Brief = normalizeText(Raw->getBriefText(Context));
  }

  std::erase(Result.Details, Result.Brief);
  return Result;
}

} // namespace astrein
