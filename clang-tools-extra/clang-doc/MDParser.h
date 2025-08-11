#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_DOC_MD_PARSER_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_DOC_MD_PARSER_H
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/Allocator.h"
#include "llvm/Support/StringSaver.h"
#include "llvm/ADT/simple_ilist.h"
#include "llvm/ADT/AllocatorList.h"

using namespace llvm;

namespace clang {
namespace doc {
enum class MDState { Paragraph, None };

enum class MDType {
  Paragraph,
  Emphasis,
  Strong,
  Text,
  Softbreak,
};

enum class MDTokenType { LeftDelimiterRun, RightDelimiterRun, Text };

template <class T> using List = simple_ilist<T, ilist_sentinel_tracking<true>>;

struct Node : ilist_node<Node, ilist_sentinel_tracking<true>> {
  List<Node> Children;
  MDType Type;
  StringRef Content;
  Node *Parent;

  Node() = default;
  Node(MDType Ty) : Type(Ty) {}
  Node(MDType Ty, StringRef Content) : Type(Ty), Content(Content) {}
  Node(MDType Ty, StringRef Content, Node *Parent) : Type(Ty), Content(Content), Parent(Parent) {}
};

struct DelimiterContext {
  // Since Content is a StringRef, we separately track the length so that we can
  // decrement when necessary without modifying the string.
  size_t Length;
  char DelimChar;
  bool RightFlanking;
  bool LeftFlanking;
  bool CanOpen;
  bool CanClose;
};

/// A LineNode might be a valid delimiter run, text, or a delimiter run that
/// will later be merged with a text if there is no matching run e.g. ***foo.
/// @brief A preprocessing structure for tracking text in a line.
struct LineNode : ilist_node<LineNode, ilist_sentinel_tracking<true>> {
  // This Twine is a concatenation of StringRefs from a line.
  Twine Content;
  // Instantiated if the line is a delimiter run.
  std::optional<DelimiterContext> DelimiterCtx;

  LineNode() : DelimiterCtx(std::nullopt) {}
  LineNode(StringRef Content) : Content(Content), DelimiterCtx(std::nullopt) {}
  LineNode(StringRef Content, DelimiterContext Ctx)
      : Content(Content), DelimiterCtx(Ctx) {
        Ctx.Length = Content.size();
      }
  LineNode(StringRef Content, std::optional<DelimiterContext> Ctx)
      : Content(Content), DelimiterCtx(Ctx) {
        if (Ctx)
          Ctx->Length = Content.size();
      }
};

using LineNodeList = BumpPtrList<LineNode>;
class MarkdownParser {
  // MDState State;
  BumpPtrAllocator Arena;
  StringSaver Saver;
  MDState State = MDState::None; 

  /// If a delimiter is found, determine if it is a delimiter run, what type of
  /// run it is, and whether it can be an opener or closer.
  ///
  /// The CommonMark specification defines delimiter runs as:
  /// A delimiter run is either a sequence of one or more * or _ characters that
  /// is not preceded or followed by a non-backslash-escaped * or _ character
  ///
  /// A left-flanking delimiter run is a delimiter run that is (1) not followed
  /// by Unicode whitespace, and either (2a) not followed by a Unicode
  /// punctuation character, or (2b) followed by a Unicode punctuation character
  /// and preceded by Unicode whitespace or a Unicode punctuation character.
  ///
  /// A right-flanking delimiter run is a delimiter run that is (1) not preceded
  /// by Unicode whitespace, and either (2a) not preceded by a Unicode
  /// punctuation character, or (2b) preceded by a Unicode punctuation character
  /// and followed by Unicode whitespace or a Unicode punctuation character.
  ///
  /// @param IdxOrigin the index of * or _ that might start a delimiter run.
  /// @return A pair denoting the type of run and the index where the run stops
  std::pair<std::optional<DelimiterContext>, size_t>
  processDelimiters(StringRef Line, const size_t &Origin = 0);

  void parseLine(StringRef Line, Node *Current);

  void processEmphasis(LineNodeList &Stack);

  void convertToNode(LineNode LN, Node *Parent);

  std::string traverse(Node &Current);

  bool isParagraph(SmallString<64> &Line);

  template <class Iterator, class ReverseIterator>
  void gatherTextNodes(Node *Parent, Iterator Start, ReverseIterator End);

  Node *determineEmphasisNode(LineNode Opener, LineNode Closer);

  /// @param Lines An entire Document that resides in a comment.
  /// @return the root of a Markdown document.
  Node* parse(std::vector<SmallString<64>> &Lines);

  void iterateAndAppend(Node &Current, std::string &Tag, std::string &Result);
public:
  MarkdownParser() : Arena(BumpPtrAllocator()), Saver(Arena) {}
  std::string render(std::vector<SmallString<64>> &Lines);
};
} // namespace doc
} // namespace clang
#endif
