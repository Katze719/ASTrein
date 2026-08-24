#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Attr.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/Expr.h"
#include "clang/AST/Mangle.h"
#include "clang/AST/RawCommentList.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/Type.h"
#include "clang/AST/TypeLoc.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Basic/Version.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/TargetParser/Host.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace {

using clang::ASTContext;
using clang::DeclaratorDecl;
using clang::FunctionDecl;
using clang::FunctionProtoType;
using clang::FunctionProtoTypeLoc;
using clang::ParmVarDecl;
using clang::QualType;
using clang::SourceLocation;
using clang::SourceManager;
using clang::Type;
using clang::TypedefNameDecl;
using clang::TypeLoc;
using clang::TypeSourceInfo;
using llvm::StringRef;

enum class OutputMode { Full, Reduced };

llvm::cl::OptionCategory AstreinCategory("ASTrein options");

llvm::cl::opt<OutputMode>
    Mode("mode", llvm::cl::desc("Output mode"),
         llvm::cl::values(
             clEnumValN(OutputMode::Full, "full",
                        "Full clang JSON AST plus callback parameter names"),
             clEnumValN(OutputMode::Reduced, "reduced",
                        "Reduced JSON API for FFI code generation")),
         llvm::cl::init(OutputMode::Full), llvm::cl::cat(AstreinCategory));

llvm::cl::opt<std::string>
    OutputPath("output", llvm::cl::desc("Output file ('-' for stdout)"),
               llvm::cl::value_desc("path"), llvm::cl::init("-"),
               llvm::cl::cat(AstreinCategory));

llvm::cl::opt<std::string> PublicHeader(
    "public-header",
    llvm::cl::desc(
        "Header spelling stored in reduced output (defaults to input)"),
    llvm::cl::value_desc("include/path.hpp"), llvm::cl::init(""),
    llvm::cl::cat(AstreinCategory));

llvm::cl::list<std::string> ApiRoots(
    "api-root",
    llvm::cl::desc("Only include declarations below this path; repeatable"),
    llvm::cl::value_desc("directory"), llvm::cl::ZeroOrMore,
    llvm::cl::cat(AstreinCategory));

struct RunState {
  bool Failed = false;
};

struct CallbackSignature {
  std::string ReturnType;
  std::vector<std::string> ParameterTypes;
  std::vector<std::optional<std::string>> ParameterNames;

  [[nodiscard]] std::size_t namedParameterCount() const {
    return std::ranges::count_if(
        ParameterNames, [](const auto &Name) { return Name.has_value(); });
  }
};

std::string printType(QualType Type, const clang::PrintingPolicy &Policy) {
  return Type.getAsString(Policy);
}

std::string pointerId(const Type *Type) {
  std::string Result;
  llvm::raw_string_ostream Stream(Result);
  Stream << static_cast<const void *>(Type);
  return Result;
}

std::string normalizedPath(StringRef Path) {
  std::string Result = Path.str();
  std::ranges::replace(Result, '\\', '/');
  while (Result.size() > 1 && Result.back() == '/')
    Result.pop_back();
#ifdef _WIN32
  std::ranges::transform(Result, Result.begin(), [](unsigned char Character) {
    return static_cast<char>(std::tolower(Character));
  });
#endif
  return Result;
}

bool pathIsBelow(StringRef Path, StringRef Root) {
  const std::string NormalPath = normalizedPath(Path);
  const std::string NormalRoot = normalizedPath(Root);
  return NormalPath == NormalRoot || (NormalPath.starts_with(NormalRoot) &&
                                      NormalPath.size() > NormalRoot.size() &&
                                      NormalPath[NormalRoot.size()] == '/');
}

std::string apiRelativePath(StringRef Path) {
  const std::string NormalPath = normalizedPath(Path);
  for (const std::string &Root : ApiRoots) {
    const std::string NormalRoot = normalizedPath(Root);
    if (!pathIsBelow(NormalPath, NormalRoot))
      continue;
    if (NormalPath.size() == NormalRoot.size())
      return {};
    return NormalPath.substr(NormalRoot.size() + 1);
  }
  return NormalPath;
}

const FunctionProtoType *functionPrototype(QualType Type) {
  for (unsigned Depth = 0; Depth != 32 && !Type.isNull(); ++Depth) {
    if (const auto *Function = Type->getAs<FunctionProtoType>())
      return Function;
    if (const auto *Pointer = Type->getAs<clang::PointerType>()) {
      Type = Pointer->getPointeeType();
      continue;
    }
    if (const auto *Reference = Type->getAs<clang::ReferenceType>()) {
      Type = Reference->getPointeeType();
      continue;
    }
    if (const auto *MemberPointer = Type->getAs<clang::MemberPointerType>()) {
      Type = MemberPointer->getPointeeType();
      continue;
    }
    if (const auto *BlockPointer = Type->getAs<clang::BlockPointerType>()) {
      Type = BlockPointer->getPointeeType();
      continue;
    }
    break;
  }
  return nullptr;
}

class SignatureCatalog {
public:
  SignatureCatalog(ASTContext &Context, clang::PrintingPolicy Policy)
      : Context(Context), Policy(std::move(Policy)) {}

  void collect(DeclaratorDecl *Declaration) {
    if (Declaration == nullptr)
      return;
    std::optional<CallbackSignature> Direct =
        collectTypeSourceInfo(Declaration->getTypeSourceInfo());
    if (auto *Parameter = llvm::dyn_cast<ParmVarDecl>(Declaration)) {
      Parameters.push_back(Parameter);
      if (Direct.has_value() &&
          functionPrototype(Parameter->getType()) != nullptr)
        ByDeclarationJsonId.insert_or_assign(pointerIdForDecl(Parameter),
                                             std::move(*Direct));
    }
  }

  void collect(TypedefNameDecl *Declaration) {
    if (Declaration == nullptr)
      return;
    std::optional<CallbackSignature> Signature =
        collectTypeSourceInfo(Declaration->getTypeSourceInfo());
    if (!Signature.has_value())
      return;
    ByTypedef.insert_or_assign(Declaration, *Signature);
    ByDeclarationJsonId.insert_or_assign(pointerIdForDecl(Declaration),
                                         std::move(*Signature));
  }

  void finalizeDeclarations() {
    for (const ParmVarDecl *Parameter : Parameters) {
      if (const CallbackSignature *Signature = lookup(*Parameter))
        ByDeclarationJsonId.insert_or_assign(pointerIdForDecl(Parameter),
                                             *Signature);
    }
  }

  [[nodiscard]] const CallbackSignature *
  lookup(const ParmVarDecl &Parameter) const {
    if (const auto Direct =
            ByDeclarationJsonId.find(pointerIdForDecl(&Parameter));
        Direct != ByDeclarationJsonId.end())
      return &Direct->second;

    QualType Current = Parameter.getType();
    for (unsigned Depth = 0; Depth != 32 && !Current.isNull(); ++Depth) {
      const Type *Raw = Current.getTypePtr();
      if (const auto *Typedef = llvm::dyn_cast<clang::TypedefType>(Raw)) {
        if (const auto Found = ByTypedef.find(Typedef->getDecl());
            Found != ByTypedef.end())
          return &Found->second;
      }
      QualType Desugared = Current.getSingleStepDesugaredType(Context);
      if (Desugared == Current)
        break;
      Current = Desugared;
    }
    return lookup(Parameter.getType());
  }

  [[nodiscard]] const std::unordered_map<std::string, CallbackSignature> &
  byDeclarationJsonId() const {
    return ByDeclarationJsonId;
  }

  [[nodiscard]] const std::unordered_map<std::string, CallbackSignature> &
  byJsonId() const {
    return ByJsonId;
  }

private:
  static std::string pointerIdForDecl(const clang::Decl *Declaration) {
    std::string Result;
    llvm::raw_string_ostream Stream(Result);
    Stream << static_cast<const void *>(Declaration);
    return Result;
  }

  std::optional<CallbackSignature> collectTypeSourceInfo(TypeSourceInfo *Info) {
    if (Info == nullptr)
      return std::nullopt;

    std::optional<CallbackSignature> Found;

    for (TypeLoc Location = Info->getTypeLoc(); !Location.isNull();
         Location = Location.getNextTypeLoc()) {
      const auto FunctionLocation = Location.getAs<FunctionProtoTypeLoc>();
      if (FunctionLocation.isNull())
        continue;

      const auto *Function = FunctionLocation.getTypePtr();
      CallbackSignature Signature;
      Signature.ReturnType = printType(Function->getReturnType(), Policy);
      Signature.ParameterTypes.reserve(Function->getNumParams());
      Signature.ParameterNames.reserve(Function->getNumParams());

      for (unsigned Index = 0; Index != Function->getNumParams(); ++Index) {
        Signature.ParameterTypes.push_back(
            printType(Function->getParamType(Index), Policy));
        const ParmVarDecl *Parameter = FunctionLocation.getParam(Index);
        if (Parameter != nullptr && !Parameter->getName().empty())
          Signature.ParameterNames.emplace_back(Parameter->getNameAsString());
        else
          Signature.ParameterNames.emplace_back(std::nullopt);
      }

      Found = Signature;
      add(Function, std::move(Signature));
    }
    return Found;
  }

  [[nodiscard]] const CallbackSignature *lookup(QualType CallbackType) const {
    const FunctionProtoType *Function = functionPrototype(CallbackType);
    if (Function == nullptr)
      return nullptr;

    if (const auto Exact = ByExactType.find(Function);
        Exact != ByExactType.end())
      return &Exact->second;

    const Type *Canonical = ASTContext::getCanonicalType(Function);
    const auto Candidates = ByCanonicalType.find(Canonical);
    if (Candidates == ByCanonicalType.end() || Candidates->second.empty())
      return nullptr;
    return &Candidates->second.front();
  }

  void add(const FunctionProtoType *Function, CallbackSignature Signature) {
    const auto Existing = ByExactType.find(Function);
    if (Existing == ByExactType.end() ||
        Existing->second.namedParameterCount() <
            Signature.namedParameterCount()) {
      ByExactType.insert_or_assign(Function, Signature);
      ByJsonId.insert_or_assign(pointerId(Function), Signature);
    }

    const Type *Canonical = ASTContext::getCanonicalType(Function);
    auto &Candidates = ByCanonicalType[Canonical];
    Candidates.push_back(std::move(Signature));
    std::ranges::stable_sort(
        Candidates, [](const auto &Left, const auto &Right) {
          return Left.namedParameterCount() > Right.namedParameterCount();
        });
  }

  ASTContext &Context;
  clang::PrintingPolicy Policy;
  std::unordered_map<const FunctionProtoType *, CallbackSignature> ByExactType;
  std::unordered_map<const Type *, std::vector<CallbackSignature>>
      ByCanonicalType;
  std::unordered_map<const TypedefNameDecl *, CallbackSignature> ByTypedef;
  std::vector<const ParmVarDecl *> Parameters;
  std::unordered_map<std::string, CallbackSignature> ByJsonId;
  std::unordered_map<std::string, CallbackSignature> ByDeclarationJsonId;
};

struct ParameterDoc {
  std::string Description;
  std::string Direction = "in";
};

struct FunctionDoc {
  std::string Brief;
  std::string Returns;
  std::vector<std::string> Details;
  std::unordered_map<std::string, ParameterDoc> Parameters;
};

std::string appendWords(std::string Current, StringRef More) {
  More = More.trim();
  if (More.empty())
    return Current;
  if (!Current.empty())
    Current.push_back(' ');
  Current.append(More.begin(), More.end());
  return Current;
}

struct CommentSection {
  enum class Kind {
    None,
    Brief,
    Returns,
    Details,
    Parameter
  } Type = Kind::None;
  std::string ParameterName;
  std::string Direction = "in";
  std::string Text;
};

void flushCommentSection(CommentSection &Section, FunctionDoc &Doc) {
  if (Section.Text.empty()) {
    Section = {};
    return;
  }

  switch (Section.Type) {
  case CommentSection::Kind::Brief:
    Doc.Brief = appendWords(std::move(Doc.Brief), Section.Text);
    break;
  case CommentSection::Kind::Returns:
    Doc.Returns = appendWords(std::move(Doc.Returns), Section.Text);
    break;
  case CommentSection::Kind::Details:
  case CommentSection::Kind::None:
    Doc.Details.push_back(std::move(Section.Text));
    break;
  case CommentSection::Kind::Parameter:
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

bool startCommentCommand(StringRef Line, CommentSection &Section) {
  Line = Line.trim();
  if (Line.empty() || (Line.front() != '@' && Line.front() != '\\'))
    return false;
  Line = Line.drop_front();

  const std::size_t CommandEnd = Line.find_first_of("[ \t");
  const StringRef Command = Line.take_front(CommandEnd);
  Line =
      CommandEnd == StringRef::npos ? StringRef{} : Line.drop_front(CommandEnd);

  if (Command == "brief") {
    Section.Type = CommentSection::Kind::Brief;
  } else if (Command == "return" || Command == "returns" ||
             Command == "result") {
    Section.Type = CommentSection::Kind::Returns;
  } else if (Command == "details" || Command == "remark" ||
             Command == "remarks" || Command == "note") {
    Section.Type = CommentSection::Kind::Details;
  } else if (Command == "param") {
    Section.Type = CommentSection::Kind::Parameter;
    Line = Line.ltrim();
    if (Line.starts_with("[")) {
      const std::size_t Close = Line.find(']');
      if (Close != StringRef::npos) {
        Section.Direction = Line.slice(1, Close).trim().str();
        Line = Line.drop_front(Close + 1).ltrim();
      }
    }
    const std::size_t NameEnd = Line.find_first_of(" \t");
    Section.ParameterName = Line.take_front(NameEnd).str();
    Line = NameEnd == StringRef::npos ? StringRef{} : Line.drop_front(NameEnd);
  } else {
    return false;
  }

  Section.Text = Line.trim().str();
  return true;
}

FunctionDoc extractDocumentation(const FunctionDecl &Declaration,
                                 ASTContext &Context) {
  FunctionDoc Result;
  const clang::RawComment *Raw =
      Context.getRawCommentForAnyRedecl(&Declaration);
  if (Raw == nullptr)
    return Result;

  const std::string Formatted = Raw->getFormattedText(
      Context.getSourceManager(), Context.getDiagnostics());
  CommentSection Current;

  StringRef Remaining(Formatted);
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
      if (Current.Type == CommentSection::Kind::None)
        Current.Type = CommentSection::Kind::Details;
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

llvm::json::Array callbackParametersJson(const CallbackSignature &Signature,
                                         bool ClangTypeObjects) {
  llvm::json::Array Parameters;
  Parameters.reserve(Signature.ParameterTypes.size());
  for (std::size_t Index = 0; Index != Signature.ParameterTypes.size();
       ++Index) {
    llvm::json::Object Parameter;
    if (Index < Signature.ParameterNames.size() &&
        Signature.ParameterNames[Index].has_value())
      Parameter["name"] = *Signature.ParameterNames[Index];
    if (ClangTypeObjects) {
      llvm::json::Object Type;
      Type["qualType"] = Signature.ParameterTypes[Index];
      Parameter["type"] = std::move(Type);
    } else {
      Parameter["type"] = Signature.ParameterTypes[Index];
    }
    Parameters.emplace_back(std::move(Parameter));
  }
  return Parameters;
}

void patchFullAst(
    llvm::json::Value &Node,
    const std::unordered_map<std::string, CallbackSignature> &TypeSignatures,
    const std::unordered_map<std::string, CallbackSignature>
        &DeclarationSignatures) {
  if (auto *Object = Node.getAsObject()) {
    const auto Kind = Object->getString("kind");
    const auto Id = Object->getString("id");
    if (Kind && Id && *Kind == "FunctionProtoType") {
      if (const auto Signature = TypeSignatures.find(Id->str());
          Signature != TypeSignatures.end()) {
        (*Object)["parameters"] =
            callbackParametersJson(Signature->second, true);
      }
    }
    if (Id) {
      if (const auto Signature = DeclarationSignatures.find(Id->str());
          Signature != DeclarationSignatures.end()) {
        (*Object)["callbackParameters"] =
            callbackParametersJson(Signature->second, true);
      }
    }
    for (auto &[Key, Child] : *Object) {
      (void)Key;
      patchFullAst(Child, TypeSignatures, DeclarationSignatures);
    }
    return;
  }

  if (auto *Array = Node.getAsArray())
    for (auto &Child : *Array)
      patchFullAst(Child, TypeSignatures, DeclarationSignatures);
}

llvm::json::Object documentationJson(const FunctionDoc &Doc) {
  llvm::json::Object Result;
  if (!Doc.Brief.empty())
    Result["brief"] = Doc.Brief;
  if (!Doc.Returns.empty())
    Result["returns"] = Doc.Returns;
  if (!Doc.Details.empty()) {
    llvm::json::Array Details;
    for (const auto &Detail : Doc.Details)
      Details.emplace_back(Detail);
    Result["details"] = std::move(Details);
  }
  return Result;
}

class AstVisitor : public clang::RecursiveASTVisitor<AstVisitor> {
public:
  AstVisitor(ASTContext &Context, SignatureCatalog &Signatures)
      : Sources(Context.getSourceManager()), Signatures(Signatures) {}

  bool VisitDeclaratorDecl(DeclaratorDecl *Declaration) {
    Signatures.collect(Declaration);
    return true;
  }

  bool VisitTypedefNameDecl(TypedefNameDecl *Declaration) {
    Signatures.collect(Declaration);
    return true;
  }

  bool VisitFunctionDecl(FunctionDecl *Declaration) {
    if (isFfiFunction(*Declaration)) {
      const FunctionDecl *Canonical = Declaration->getCanonicalDecl();
      if (SeenFunctions.insert(Canonical).second)
        Functions.push_back(Declaration);
    }
    return true;
  }

  [[nodiscard]] const std::vector<const FunctionDecl *> &functions() const {
    return Functions;
  }

private:
  bool isFfiFunction(const FunctionDecl &Declaration) const {
    if (Declaration.isImplicit() || Declaration.getLocation().isInvalid() ||
        llvm::isa<clang::CXXMethodDecl>(&Declaration) ||
        Declaration.getTemplatedKind() != FunctionDecl::TK_NonTemplate ||
        Declaration.getIdentifier() == nullptr ||
        !Declaration.hasExternalFormalLinkage())
      return false;

    const SourceLocation Location =
        Sources.getExpansionLoc(Declaration.getLocation());
    if (Sources.isInSystemHeader(Location))
      return false;

    if (ApiRoots.empty())
      return true;

    const clang::PresumedLoc Presumed = Sources.getPresumedLoc(Location);
    if (Presumed.isInvalid())
      return false;
    const StringRef Filename(Presumed.getFilename());
    return std::ranges::any_of(ApiRoots, [&](const std::string &Root) {
      return pathIsBelow(Filename, Root);
    });
  }

  SourceManager &Sources;
  SignatureCatalog &Signatures;
  std::unordered_set<const FunctionDecl *> SeenFunctions;
  std::vector<const FunctionDecl *> Functions;
};

std::string expressionText(const clang::Expr &Expression,
                           const clang::PrintingPolicy &Policy) {
  std::string Result;
  llvm::raw_string_ostream Stream(Result);
  Expression.printPretty(Stream, nullptr, Policy);
  return Result;
}

std::string declaredIn(const FunctionDecl &Declaration,
                       const SourceManager &Sources) {
  const clang::PresumedLoc Location =
      Sources.getPresumedLoc(Declaration.getLocation());
  return Location.isValid() ? apiRelativePath(Location.getFilename())
                            : std::string{};
}

llvm::json::Object callbackJson(const CallbackSignature &Signature) {
  llvm::json::Object Callback;
  Callback["returnType"] = Signature.ReturnType;
  Callback["parameters"] = callbackParametersJson(Signature, false);
  return Callback;
}

llvm::json::Object reducedFunctionJson(const FunctionDecl &Declaration,
                                       ASTContext &Context,
                                       const SignatureCatalog &Signatures,
                                       clang::ASTNameGenerator &Names,
                                       const clang::PrintingPolicy &Policy) {
  llvm::json::Object Function;
  Function["name"] = Declaration.getQualifiedNameAsString();
  Function["symbol"] = Names.getName(&Declaration);
  Function["declaredIn"] = declaredIn(Declaration, Context.getSourceManager());
  Function["returnType"] = printType(Declaration.getReturnType(), Policy);

  const FunctionDoc Docs = extractDocumentation(Declaration, Context);
  llvm::json::Array Parameters;
  Parameters.reserve(Declaration.getNumParams());
  for (const ParmVarDecl *Parameter : Declaration.parameters()) {
    llvm::json::Object JsonParameter;
    JsonParameter["name"] = Parameter->getNameAsString();
    JsonParameter["type"] = printType(Parameter->getType(), Policy);

    if (Parameter->hasDefaultArg() && Parameter->getDefaultArg() != nullptr)
      JsonParameter["default"] =
          expressionText(*Parameter->getDefaultArg(), Policy);

    if (const CallbackSignature *Callback = Signatures.lookup(*Parameter))
      JsonParameter["callback"] = callbackJson(*Callback);

    if (const auto Doc = Docs.Parameters.find(Parameter->getNameAsString());
        Doc != Docs.Parameters.end()) {
      llvm::json::Object JsonDoc;
      if (!Doc->second.Description.empty())
        JsonDoc["description"] = Doc->second.Description;
      JsonDoc["direction"] = Doc->second.Direction;
      JsonParameter["doc"] = std::move(JsonDoc);
    }
    Parameters.emplace_back(std::move(JsonParameter));
  }
  Function["parameters"] = std::move(Parameters);

  if (Declaration.isInlineSpecified() || Declaration.isInlined())
    Function["inline"] = true;

  llvm::json::Object JsonDocs = documentationJson(Docs);
  if (!JsonDocs.empty())
    Function["doc"] = std::move(JsonDocs);
  return Function;
}

class AstConsumer final : public clang::ASTConsumer {
public:
  AstConsumer(llvm::raw_ostream &Output, OutputMode Mode,
              std::string PublicHeader, RunState &State,
              clang::CompilerInstance &Compiler)
      : Output(Output), Mode(Mode), PublicHeader(std::move(PublicHeader)),
        State(State), Compiler(Compiler) {}

  void HandleTranslationUnit(ASTContext &Context) override {
    clang::PrintingPolicy Policy(Context.getLangOpts());
    Policy.SuppressScope = false;

    SignatureCatalog Signatures(Context, Policy);
    AstVisitor Visitor(Context, Signatures);
    Visitor.TraverseDecl(Context.getTranslationUnitDecl());
    Signatures.finalizeDeclarations();

    if (Mode == OutputMode::Full)
      writeFull(Context, Signatures);
    else
      writeReduced(Context, Signatures, Visitor.functions(), Policy);
  }

private:
  void fail(StringRef Message) {
    State.Failed = true;
    const unsigned Diagnostic = Compiler.getDiagnostics().getCustomDiagID(
        clang::DiagnosticsEngine::Error, "%0");
    Compiler.getDiagnostics().Report(Diagnostic) << Message;
  }

  void writeFull(ASTContext &Context, const SignatureCatalog &Signatures) {
    std::string Dump;
    llvm::raw_string_ostream DumpStream(Dump);
    Context.getTranslationUnitDecl()->dump(DumpStream, false, clang::ADOF_JSON);
    DumpStream.flush();

    llvm::Expected<llvm::json::Value> Parsed = llvm::json::parse(Dump);
    if (!Parsed) {
      fail(llvm::toString(Parsed.takeError()));
      return;
    }

    patchFullAst(*Parsed, Signatures.byJsonId(),
                 Signatures.byDeclarationJsonId());
    Output << llvm::formatv("{0:2}", *Parsed) << '\n';
  }

  void writeReduced(ASTContext &Context, const SignatureCatalog &Signatures,
                    std::vector<const FunctionDecl *> Functions,
                    const clang::PrintingPolicy &Policy) {
    const SourceManager &Sources = Context.getSourceManager();
    std::ranges::stable_sort(
        Functions, [&](const FunctionDecl *Left, const FunctionDecl *Right) {
          return Sources.isBeforeInTranslationUnit(Left->getLocation(),
                                                   Right->getLocation());
        });

    llvm::json::Object Root;
    Root["schema"] = "cpp_core_ffi_api";
    Root["schemaVersion"] = 1;
    Root["publicHeader"] = PublicHeader;

    clang::ASTNameGenerator NameGenerator(Context);
    llvm::json::Array JsonFunctions;
    JsonFunctions.reserve(Functions.size());
    for (const FunctionDecl *Function : Functions)
      JsonFunctions.emplace_back(reducedFunctionJson(
          *Function, Context, Signatures, NameGenerator, Policy));
    Root["functions"] = std::move(JsonFunctions);
    Output << llvm::formatv("{0:2}", llvm::json::Value(std::move(Root)))
           << '\n';
  }

  llvm::raw_ostream &Output;
  OutputMode Mode;
  std::string PublicHeader;
  RunState &State;
  clang::CompilerInstance &Compiler;
};

class AstFrontendAction final : public clang::ASTFrontendAction {
public:
  AstFrontendAction(llvm::raw_ostream &Output, OutputMode Mode,
                    std::string PublicHeader, RunState &State)
      : Output(Output), Mode(Mode), PublicHeader(std::move(PublicHeader)),
        State(State) {}

  std::unique_ptr<clang::ASTConsumer>
  CreateASTConsumer(clang::CompilerInstance &Compiler,
                    StringRef /*InputFile*/) override {
    return std::make_unique<AstConsumer>(Output, Mode, PublicHeader, State,
                                         Compiler);
  }

private:
  llvm::raw_ostream &Output;
  OutputMode Mode;
  std::string PublicHeader;
  RunState &State;
};

class AstActionFactory final : public clang::tooling::FrontendActionFactory {
public:
  AstActionFactory(llvm::raw_ostream &Output, OutputMode Mode,
                   std::string PublicHeader, RunState &State)
      : Output(Output), Mode(Mode), PublicHeader(std::move(PublicHeader)),
        State(State) {}

  std::unique_ptr<clang::FrontendAction> create() override {
    return std::make_unique<AstFrontendAction>(Output, Mode, PublicHeader,
                                               State);
  }

private:
  llvm::raw_ostream &Output;
  OutputMode Mode;
  std::string PublicHeader;
  RunState &State;
};

} // namespace

int main(int argc, const char **argv) {
  llvm::cl::SetVersionPrinter([](llvm::raw_ostream &Stream) {
    Stream << "ASTrein " ASTREIN_VERSION << '\n'
           << "Clang " CLANG_VERSION_STRING << '\n';
  });

  auto Options = clang::tooling::CommonOptionsParser::create(
      argc, argv, AstreinCategory, llvm::cl::OneOrMore);
  if (!Options) {
    llvm::errs() << llvm::toString(Options.takeError()) << '\n';
    return 2;
  }

  const auto &Sources = Options->getSourcePathList();
  if (Sources.size() != 1) {
    llvm::errs() << "astrein: exactly one translation unit is required\n";
    return 2;
  }

  for (std::string &Root : ApiRoots) {
    llvm::SmallString<256> Absolute(Root);
    if (std::error_code Error = llvm::sys::fs::make_absolute(Absolute)) {
      llvm::errs() << "astrein: cannot resolve API root '" << Root
                   << "': " << Error.message() << '\n';
      return 2;
    }
    llvm::sys::path::remove_dots(Absolute, true);
    Root = Absolute.str().str();
  }

  std::unique_ptr<llvm::raw_fd_ostream> FileOutput;
  llvm::raw_ostream *Output = &llvm::outs();
  if (OutputPath != "-") {
    std::error_code Error;
    FileOutput = std::make_unique<llvm::raw_fd_ostream>(OutputPath, Error,
                                                        llvm::sys::fs::OF_Text);
    if (Error) {
      llvm::errs() << "astrein: cannot open '" << OutputPath
                   << "': " << Error.message() << '\n';
      return 2;
    }
    Output = FileOutput.get();
  }

  std::string Header = PublicHeader;
  if (Header.empty())
    Header = apiRelativePath(Sources.front());

  RunState State;
  clang::tooling::ClangTool Tool(Options->getCompilations(), Sources);
  if (llvm::sys::getDefaultTargetTriple().empty()) {
    clang::tooling::CommandLineArguments HostTarget{
        "--target=" + llvm::sys::getProcessTriple()};
    Tool.appendArgumentsAdjuster(clang::tooling::getInsertArgumentAdjuster(
        HostTarget, clang::tooling::ArgumentInsertPosition::BEGIN));
  }
  Tool.appendArgumentsAdjuster(clang::tooling::getClangSyntaxOnlyAdjuster());
  Tool.appendArgumentsAdjuster(clang::tooling::getInsertArgumentAdjuster(
      "-fparse-all-comments", clang::tooling::ArgumentInsertPosition::END));

  AstActionFactory Factory(*Output, Mode, std::move(Header), State);
  const int Result = Tool.run(&Factory);
  Output->flush();
  return Result == 0 && !State.Failed ? 0 : 1;
}
