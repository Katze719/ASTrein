#pragma once

#include "llvm/ADT/StringRef.h"

namespace astrein {

struct CommentSection;

[[nodiscard]] bool startCommentCommand(llvm::StringRef Line,
                                       CommentSection &Section);

} // namespace astrein
