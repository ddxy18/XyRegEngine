//
// Created by dxy on 2020/8/7.
//

#include <queue>

#include "gtest/gtest.h"
#include "lex.h"

using namespace XyRegEngine;
using namespace std;

void LexTest(const string &regex, queue<string> q) {
    string lex;
    auto begin = regex.cbegin(), end = regex.cend();
    while (!(lex = NextToken(begin, end)).empty()) {
        EXPECT_EQ(lex, q.front());
        q.pop();
    }
}

TEST(Lexer, Range) {
    string regex = "[A-Za-z_][A-Za-z0-9_]*";
    queue<string> q;
    q.push("[A-Za-z_]");
    q.push("[A-Za-z0-9_]");
    q.push("*");

    LexTest(regex, q);
}

TEST(Lexer, EscapeCharacter) {
    string regex = "\\w\\.cpp";
    queue<string> q;
    q.push("\\w");
    q.push("\\.");
    q.push("c");
    q.push("p");
    q.push("p");

    LexTest(regex, q);
}

TEST(Lexer, Parentheses) {
    string regex = "^\\w\\.(cpp|c)";
    queue<string> q;
    q.push("^");
    q.push("\\w");
    q.push("\\.");
    q.push("(cpp|c)");

    LexTest(regex, q);
}

TEST(Lexer, NestedParentheses) {
    string regex = "((0x|0X)[0-9a-fA-F]+)(u|U|l|L)*";
    queue<string> q;
    q.push("((0x|0X)[0-9a-fA-F]+)");
    q.push("(u|U|l|L)");
    q.push("*");

    LexTest(regex, q);
}

TEST(Lexer, LackOfLeftPair) {
    string regex = "^\\w\\.cpp|c)";
    queue<string> q;
    q.push("^");
    q.push("\\w");
    q.push("\\.");
    q.push("c");
    q.push("p");
    q.push("p");
    q.push("|");
    q.push("c");

    LexTest(regex, q);
}

TEST(Lexer, LackOfRightPair) {
    string regex = "^\\w\\.(cpp|c";
    queue<string> q;
    q.push("^");
    q.push("\\w");
    q.push("\\.");

    LexTest(regex, q);
}

TEST(Lexer, EscapeParentheses) {
    string regex = R"((\(\w\))+)";
    queue<string> q;
    q.push(R"((\(\w\)))");
    q.push("+");

    LexTest(regex, q);
}

TEST(Lexer, InvalidEscapeCharacter) {
    string regex = "\\";
    queue<string> q;

    LexTest(regex, q);
}

TEST(Lexer, NonGreedy) {
    string regex = "a+?ab";
    queue<string> q;
    q.push("a");
    q.push("+?");
    q.push("a");
    q.push("b");

    LexTest(regex, q);
}