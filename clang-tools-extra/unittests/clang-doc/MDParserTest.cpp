#include "MDParser.h"
#include "ClangDocTest.h"

namespace clang {
namespace doc {
TEST(MDParserTest, Strong) {
  MarkdownParser Parser;
  std::vector<SmallString<64>> Line = {{"**Strong**"}};
  auto Result = Parser.render(Line);
  std::string Expected = R"raw(<p><strong>Strong</strong></p>)raw";
  EXPECT_EQ(Expected, Result);
}

TEST(MDParserTest, StrongPrefix) {
  MarkdownParser Parser;
  std::vector<SmallString<64>> Line = {{"Very **Strong**"}};
  auto Result = Parser.render(Line);
  std::string Expected = R"raw(<p>Very <strong>Strong</strong></p>)raw";
  EXPECT_EQ(Expected, Result);
}

TEST(MDParserTest, StrongSuffix) {
  MarkdownParser Parser;
  std::vector<SmallString<64>> Line = {{"**Strong** Text"}};
  auto Result = Parser.render(Line);
  std::string Expected = R"raw(<p>Very <strong>Strong</strong> Text</p>)raw";
  EXPECT_EQ(Expected, Result);
}

// TEST(MDParserTest, DoubleStrong) {
//   MarkdownParser Parser;
//   std::vector<SmallString<64>> Line = {{"****Strong****"}};
//   auto Result = Parser.render(Line);
//   std::string Expected = R"raw(<strong><strong>Strong</strong></strong>)raw";
//   EXPECT_EQ(Expected, Result);
// }

// TEST(MDParserTest, ThreeAsterisks) {
//   MarkdownParser Parser;
//   std::vector<SmallString<64>> Line = {{"***StrongEmphasis***"}};
//   auto Result = Parser.render(Line);
//   std::string Expected = R"raw(<p><strong><em>StrongEmphasis</em></strong></p>)raw";
//   EXPECT_EQ(Expected, Result);
// }

TEST(MDParserTest, NoEmphasis) {
  MarkdownParser Parser;
  std::vector<SmallString<64>> Line = {{"**NoEmphasis*"}};
  auto Result = Parser.render(Line);
  std::string Expected = R"raw(<p>**NoEmphasis*</p>)raw";
  EXPECT_EQ(Expected, Result);
}

TEST(MDParserTest, NoEmphasisMoreText) {
  MarkdownParser Parser;
  std::vector<SmallString<64>> Line = {{"**NoEmphasis* Extra"}};
  auto Result = Parser.render(Line);
  std::string Expected = R"raw(<p>**NoEmphasis* Extra</p>)raw";
  EXPECT_EQ(Expected, Result);
}

TEST(MDParserTest, Emphasis) {
  MarkdownParser Parser;
  std::vector<SmallString<64>> Line = {{"*Emphasis*"}};
  auto Result = Parser.render(Line);
  std::string Expected = R"raw(<p><em>Emphasis</em></p>)raw";
  EXPECT_EQ(Expected, Result);
}

TEST(MDParserTest, Text) {
  MarkdownParser Parser;
  std::vector<SmallString<64>> Line = {{"Text"}};
  auto Result = Parser.render(Line);
  std::string Expected = R"raw(<p>Text</p>)raw";
  EXPECT_EQ(Expected, Result);
}
} // namespace doc
} // namespace clang
