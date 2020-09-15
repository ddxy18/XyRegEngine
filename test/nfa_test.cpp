//
// Created by dxy on 2020/8/10.
//

#include "gtest/gtest.h"
#include "nfa.h"

using namespace XyRegEngine;
using namespace std;

TEST(Nfa, Alternative) {
    Nfa nfa{"a|b"};
    string s = "abc";
    auto begin = s.cbegin(), end = s.cend();

    EXPECT_EQ(nfa.NextMatch(begin, end), "a");
    EXPECT_EQ(nfa.NextMatch(begin, end), "b");
    EXPECT_TRUE(nfa.NextMatch(begin, end).empty());
}

TEST(Nfa, And) {
    Nfa nfa{"ab"};
    string s = "abc";
    auto begin = s.cbegin(), end = s.cend();

    EXPECT_EQ(nfa.NextMatch(begin, end), "ab");
    EXPECT_TRUE(nfa.NextMatch(begin, end).empty());
}

TEST(Nfa, Range) {
    Nfa nfa{"[a-c]"};
    string s = "abc";
    auto begin = s.cbegin(), end = s.cend();

    EXPECT_EQ(nfa.NextMatch(begin, end), "a");
    EXPECT_EQ(nfa.NextMatch(begin, end), "b");
    EXPECT_EQ(nfa.NextMatch(begin, end), "c");
    EXPECT_TRUE(nfa.NextMatch(begin, end).empty());
}

TEST(Nfa, Quantifier_0OrMore) {
    Nfa nfa{"[a-c]*"};
    string s = "abc";
    auto begin = s.cbegin(), end = s.cend();

    EXPECT_EQ(nfa.NextMatch(begin, end), "abc");
    EXPECT_TRUE(nfa.NextMatch(begin, end).empty());
}

TEST(Nfa, Quantifier_1OrMore) {
    Nfa nfa{"[a-c]+"};
    string s = "abcd";
    auto begin = s.cbegin(), end = s.cend();

    EXPECT_EQ(nfa.NextMatch(begin, end), "abc");
    EXPECT_TRUE(nfa.NextMatch(begin, end).empty());
}

TEST(Nfa, Quantifier_Exact) {
    Nfa nfa{"[a-c]{2}"};
    string s = "abcd";
    auto begin = s.cbegin(), end = s.cend();

    EXPECT_EQ(nfa.NextMatch(begin, end), "ab");
    EXPECT_TRUE(nfa.NextMatch(begin, end).empty());
}

TEST(Nfa, Quantifier_nOrMore) {
    Nfa nfa{"[a-c]{2,}"};
    string s = "abcd";
    auto begin = s.cbegin(), end = s.cend();

    EXPECT_EQ(nfa.NextMatch(begin, end), "abc");
    EXPECT_TRUE(nfa.NextMatch(begin, end).empty());
}

TEST(Nfa, Quantifier_mTon) {
    Nfa nfa{"[a-c]{2,4}"};
    string s = "abcabd";
    auto begin = s.cbegin(), end = s.cend();

    EXPECT_EQ(nfa.NextMatch(begin, end), "abca");
    EXPECT_TRUE(nfa.NextMatch(begin, end).empty());
}

TEST(Nfa, Assertion_PositiveLookahead) {
    Nfa nfa{"(?=a)ab"};
    string s = "ab";
    auto begin = s.cbegin(), end = s.cend();

    EXPECT_EQ(nfa.NextMatch(begin, end), "ab");
    EXPECT_TRUE(nfa.NextMatch(begin, end).empty());
}

TEST(Nfa, Assertion_NegativeLookahead) {
    Nfa nfa{"(?!abd)abc"};
    string s = "abc";
    auto begin = s.cbegin(), end = s.cend();

    EXPECT_EQ(nfa.NextMatch(begin, end), "abc");
    EXPECT_TRUE(nfa.NextMatch(begin, end).empty());
}

TEST(Nfa, ContinuousAssertion) {
    Nfa nfa{"(?!abd)(?=abc)abc"};
    string s = "abcabc";
    auto begin = s.cbegin(), end = s.cend();

    EXPECT_EQ(nfa.NextMatch(begin, end), "abc");
    EXPECT_EQ(nfa.NextMatch(begin, end), "abc");
    EXPECT_TRUE(nfa.NextMatch(begin, end).empty());
}

TEST(Nfa, Assertion_LineBegin) {
    Nfa nfa{"^a+"};
    string s = "aaa";
    auto begin = s.cbegin(), end = s.cend();

    EXPECT_EQ(nfa.NextMatch(begin, end), "aaa");
    EXPECT_TRUE(nfa.NextMatch(begin, end).empty());
}

TEST(Nfa, Assertion_LineEnd) {
    Nfa nfa{"a+$"};
    string s = "aaa";
    auto begin = s.cbegin(), end = s.cend();

    EXPECT_EQ(nfa.NextMatch(begin, end), "aaa");
    EXPECT_TRUE(nfa.NextMatch(begin, end).empty());
}

TEST(Nfa, Assertion_WordBoundary) {
    Nfa nfa{"\\ba+\\b"};
    string s = "aaa aaa";
    auto begin = s.cbegin(), end = s.cend();

    EXPECT_EQ(nfa.NextMatch(begin, end), "aaa");
    EXPECT_TRUE(nfa.NextMatch(begin, end).empty());
}

TEST(Nfa, Assertion_NotWordBoundary) {
    Nfa nfa{"aa\\Ba"};
    string s = "aaa";
    auto begin = s.cbegin(), end = s.cend();

    EXPECT_EQ(nfa.NextMatch(begin, end), "aaa");
    EXPECT_TRUE(nfa.NextMatch(begin, end).empty());
}

TEST(Nfa, Group) {
    Nfa nfa{"(aa)ab"};
    string s = "aaabc";
    auto begin = s.cbegin(), end = s.cend();

    EXPECT_EQ(nfa.NextMatch(begin, end), "aaab");
    EXPECT_TRUE(nfa.NextMatch(begin, end).empty());
}

TEST(Nfa, PassiveGroup) {
    Nfa nfa{"(?:aa)ab"};
    string s = "aaabc";
    auto begin = s.cbegin(), end = s.cend();

    EXPECT_EQ(nfa.NextMatch(begin, end), "aaab");
    EXPECT_TRUE(nfa.NextMatch(begin, end).empty());
}

TEST(Nfa, NestedGroup) {
    Nfa nfa{"(^aa(ab))c"};
    string s = "aaabc";
    auto begin = s.cbegin(), end = s.cend();

    EXPECT_EQ(nfa.NextMatch(begin, end), "aaabc");
    EXPECT_TRUE(nfa.NextMatch(begin, end).empty());
}

TEST(Nfa, BackReference) {
    Nfa nfa{"(a*)bc\\1"};
    string s = "aabcaaa";
    auto begin = s.cbegin(), end = s.cend();

    EXPECT_EQ(nfa.NextMatch(begin, end), "aabcaa");
    EXPECT_TRUE(nfa.NextMatch(begin, end).empty());
}

TEST(Nfa, SeveralBackReference) {
    Nfa nfa{R"((a*)(b*)c\1\1\2)"};
    string s = "aabcaaaab";
    auto begin = s.cbegin(), end = s.cend();

    EXPECT_EQ(nfa.NextMatch(begin, end), "aabcaaaab");
    EXPECT_TRUE(nfa.NextMatch(begin, end).empty());
}

TEST(Nfa, NotNewLine) {
    Nfa nfa{"..."};
    string s = "aaa";
    auto begin = s.cbegin(), end = s.cend();

    EXPECT_EQ(nfa.NextMatch(begin, end), "aaa");
    EXPECT_TRUE(nfa.NextMatch(begin, end).empty());
}

TEST(Nfa, EscapeCharacter) {
    Nfa nfa{"\\(a+\\)"};
    string s = "(a)";
    auto begin = s.cbegin(), end = s.cend();

    EXPECT_EQ(nfa.NextMatch(begin, end), "(a)");
    EXPECT_TRUE(nfa.NextMatch(begin, end).empty());
}

TEST(Nfa, SpecialPatternInRange) {
    Nfa nfa{"[\\w]"};
    string s = "a1";
    auto begin = s.cbegin(), end = s.cend();

    EXPECT_EQ(nfa.NextMatch(begin, end), "a");
    EXPECT_EQ(nfa.NextMatch(begin, end), "1");
    EXPECT_TRUE(nfa.NextMatch(begin, end).empty());
}