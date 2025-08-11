#include "MDParser.h"
#include "clang/Basic/CharInfo.h"
#include "llvm/ADT/AllocatorList.h"

namespace clang {
namespace doc {
namespace {
bool isEmphasisDelimiter(char Token) {
  // TODO: support '_'
  if (Token == '*')
    return true;
  return false;
}
} // namespace

std::pair<std::optional<DelimiterContext>, size_t>
MarkdownParser::processDelimiters(StringRef Line, const size_t &Start) {
  size_t Idx = Start;
  while (Idx < Line.size() && Line[Idx] == Line[Start]) {
    ++Idx;
  }
  // size_t DelimiterRunLength = Idx - Origin;

  char Preceeding = (Start == 0) ? ' ' : Line[Start - 1];
  char Proceeding = (Idx >= Line.size()) ? ' ' : Line[Idx];

  bool LeftFlanking = !isWhitespace(Proceeding) &&
                      (!isPunctuation(Proceeding) || isWhitespace(Preceeding) ||
                       isPunctuation(Preceeding));
  bool RightFlanking = !isWhitespace(Preceeding) &&
                       (!isPunctuation(Preceeding) ||
                        isWhitespace(Proceeding) || isPunctuation(Proceeding));

  if (LeftFlanking && RightFlanking)
    return {DelimiterContext{LeftFlanking, RightFlanking, true, true}, Idx};
  if (LeftFlanking)
    return {DelimiterContext{LeftFlanking, RightFlanking, true, false}, Idx};
  if (RightFlanking)
    return {DelimiterContext{LeftFlanking, RightFlanking, false, true}, Idx};
  return {std::nullopt, 0};
}

bool MarkdownParser::isParagraph(SmallString<64> &Line) {
  if (Line.size() > 1 && (Line[0] == '-' || Line[0] == '*') && Line[1] == ' ')
    return false;
  return true;
}

template <class Iterator, class ReverseIterator>
void MarkdownParser::gatherTextNodes(Node *Parent, Iterator Start,
                                     ReverseIterator End) {
  Twine Combined;
  auto EndIt = End->getIterator();
  while (Start != EndIt) {
    Combined.concat(Start->Content);
    Start++;
  }
  auto *NewNode = new (Arena) Node(MDType::Text, Saver.save(Combined), Parent);
  Parent->Children.push_back(*NewNode);
}

Node *MarkdownParser::determineEmphasisNode(LineNode Opener, LineNode Closer) {
  if (!Opener.DelimiterCtx || !Closer.DelimiterCtx) {
    return nullptr;
  }
  auto &OpenerLength = Opener.DelimiterCtx->Length;
  auto &CloserLength = Closer.DelimiterCtx->Length;

  // if (OpenerLength % 3 == 0 && CloserLength % 3 == 0) {
  //   // We know that this will be many strong emphasis
  // }

  // if (OpenerLength % 3 == 1 && CloserLength % 3 == 1) {
  //   // We know that this will be many strongs with 1 emphasis
  // }

  Node *PossibleNode;
  if (OpenerLength >= 2 && CloserLength >= 2) {
    PossibleNode = new (Arena) Node(MDType::Strong);
    OpenerLength -= 2;
    CloserLength -= 2;
    return PossibleNode;
  }

  if (OpenerLength == 1 && CloserLength == 1) {
    PossibleNode = new (Arena) Node(MDType::Emphasis);
    OpenerLength -= 1;
    CloserLength -= 1;
    return PossibleNode;
  }

  return nullptr;
}

void MarkdownParser::processEmphasis(LineNodeList &Stack) {
  // Use the node's iterator, not the list's
  auto It = Stack.begin()->getIterator();

  if (It.isEnd())
    return;

  while (!It.isEnd()) {
    LineNode Current = *It;

    if (Current.DelimiterCtx && Current.DelimiterCtx->CanClose) {
      // Iterate in reverse to try to find the most recent opener.
      auto ReverseIt = It->getReverseIterator();
      while (!ReverseIt.isEnd()) {
        if (ReverseIt->DelimiterCtx && ReverseIt->DelimiterCtx->CanOpen)
          break;
        ++ReverseIt;
      }

      if (!ReverseIt.isEnd() && (*It).DelimiterCtx &&
          (*ReverseIt).DelimiterCtx) {
        auto *NewNode = determineEmphasisNode(*It, *ReverseIt);
        if (!NewNode) {
          ++It;
          continue;
        }
        gatherTextNodes(NewNode, It, ReverseIt);
        // Result.push_back(NewNode);
        // Stack.erase(It, ReverseIt);
      }
    } else if (!Current.DelimiterCtx) {
      // auto *TextNode = new (Arena) Node();
      // TextNode->Type = MDType::Text;
      // TextNode->Content = Current.Content;
      // Result.push_back(TextNode);
      // It = Stack.erase(It);
    }
    ++It;
  }

  // for (auto *LeftoverNode : Stack) {
  //   auto *TextNode = new (Arena) Node();
  //   TextNode->Type = MDType::Text;
  //   TextNode->Content = LeftoverNode->Content;
  //   Result.push_back(TextNode);
  // }

  // return Result;
}

void MarkdownParser::parseLine(StringRef Line, Node *Current) {
  LineNodeList Stack;
  // List<LineNode> Stack;
  size_t Idx = 0;
  Twine PotentialTextNodeContent;
  for (; Idx < Line.size(); ++Idx) {
    if (isEmphasisDelimiter(Line[Idx])) {
      auto DelimiterResult = processDelimiters(Line, Idx);
      if (DelimiterResult.first) {
        Stack.emplace_back(Line.substr(Idx, DelimiterResult.second - Idx), DelimiterResult.first);
        Idx = DelimiterResult.second - 1;
        continue;
      }
    }

    if (Stack.empty())
      Stack.emplace_back(Line.substr(Idx));
    else
      Stack.back().Content.concat(Line.substr(Idx));
  }

  processEmphasis(Stack);
}

Node *MarkdownParser::parse(std::vector<SmallString<64>> &Lines) {
  auto *Root = new (Arena) Node();
  auto *Current = Root;
  for (auto &Line : Lines) {
    if (State != MDState::Paragraph && isParagraph(Line)) {
      State = MDState::Paragraph;
      auto *NewNode = new (Arena) Node();
      NewNode->Type = MDType::Paragraph;
      Current->Children.push_back(*NewNode);
      NewNode->Parent = Current;
      Current = NewNode;
    }

    if (Line.empty()) {
      State = MDState::None;
      continue;
    }
    parseLine(Line, Current);
  }
  return Root;
}

void MarkdownParser::iterateAndAppend(Node &Current, std::string &Tag, std::string &Result) {
  Result.append("<" + Tag + ">");
  for (auto &Child : Current.Children)
    Result.append(traverse(Child));
  Result.append("</" + Tag + ">");
}

std::string MarkdownParser::traverse(Node &Current) {
  std::string Result;
  std::string Tag;
  switch (Current.Type) {
  case MDType::Strong:
    Tag = "strong";
    iterateAndAppend(Current, Tag, Result);
    break;
  case MDType::Text:
    Result.append(Current.Content);
    break;
  case MDType::Softbreak:
    Result.append("\n");
    break;
  case MDType::Paragraph:
  Tag = "p";
    iterateAndAppend(Current, Tag, Result);
    break;
  case MDType::Emphasis:
    Tag = "p";
    iterateAndAppend(Current, Tag, Result);
    break;
  }
  return Result;
}

std::string MarkdownParser::render(std::vector<SmallString<64>> &Lines) {
  auto *Document = parse(Lines);
  std::string Result;
  for (auto &Child : Document->Children)
    Result.append(traverse(Child));
  return Result;
}
} // namespace doc
} // namespace clang
