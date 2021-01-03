#include "src/Lexer.h"

#include <gtest/gtest.h>

namespace TXL
{
using namespace testing;

TEST(LexerTestSuite, parseSQRT)
{
    const std::string str = "$$\\sqrt[3]{(x-y)^4}=x+y$$";
    Lexer lexer(str);

    EXPECT_EQ((Token{COMMAND, "sqrt"}), lexer.next());
    EXPECT_EQ((Token{START_GROUP, "["}), lexer.next());
    EXPECT_EQ((Token{TEXT, "3"}), lexer.next());
    EXPECT_EQ((Token{END_GROUP, "]"}), lexer.next());
    EXPECT_EQ((Token{START_GROUP, "{"}), lexer.next());
    EXPECT_EQ((Token{TEXT, "(x"}), lexer.next());
    EXPECT_EQ((Token{SIGN, "-"}), lexer.next());
    EXPECT_EQ((Token{TEXT, "y)"}), lexer.next());
    EXPECT_EQ((Token{SIGN, "^"}), lexer.next());
    EXPECT_EQ((Token{TEXT, "4"}), lexer.next());
    EXPECT_EQ((Token{END_GROUP, "}"}), lexer.next());
    EXPECT_EQ((Token{SIGN, "="}), lexer.next());
    EXPECT_EQ((Token{TEXT, "x"}), lexer.next());
    EXPECT_EQ((Token{SIGN, "+"}), lexer.next());
    EXPECT_EQ((Token{TEXT, "y"}), lexer.next());
}

TEST(LexerTestSuite, parseEscapedSymbol)
{
    const std::string str = "\\{\\}";
    Lexer lexer(str);

    EXPECT_EQ((Token{TEXT, "{"}), lexer.next());
    EXPECT_EQ((Token{TEXT, "}"}), lexer.next());
}
} // namespace TXL
