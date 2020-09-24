//
// Created by dxy on 2020/8/10.
//

#include "gtest/gtest.h"
#include "nfa.h"

using namespace XyRegEngine;
using namespace std;

TEST(Nfa, Alternative) {
    Nfa nfa{"a|b"};
    string s = "ab";
    auto begin = s.cbegin(), end = s.cend();

    auto match_end = nfa.NextMatch(begin, end)->first.second;
    EXPECT_EQ(string(begin, match_end), "a");

    begin = match_end;
    match_end = nfa.NextMatch(begin, end)->first.second;
    EXPECT_EQ(string(begin, match_end), "b");

    begin = match_end;
    EXPECT_EQ(nfa.NextMatch(begin, end), nullptr);
}

TEST(Nfa, And) {
    Nfa nfa{"ab"};
    string s = "abc";
    auto begin = s.cbegin(), end = s.cend();

    auto match_end = nfa.NextMatch(begin, end)->first.second;
    EXPECT_EQ(string(begin, match_end), "ab");

    begin = match_end;
    EXPECT_EQ(nfa.NextMatch(begin, end), nullptr);
}

TEST(Nfa, Range) {
    Nfa nfa{"[a-c]"};
    string s = "abc";
    auto begin = s.cbegin(), end = s.cend();

    auto match_end = nfa.NextMatch(begin, end)->first.second;
    EXPECT_EQ(string(begin, match_end), "a");

    begin = match_end;
    match_end = nfa.NextMatch(begin, end)->first.second;
    EXPECT_EQ(string(begin, match_end), "b");

    begin = match_end;
    match_end = nfa.NextMatch(begin, end)->first.second;
    EXPECT_EQ(string(begin, match_end), "c");

    begin = match_end;
    EXPECT_EQ(nfa.NextMatch(begin, end), nullptr);
}

TEST(Nfa, Quantifier_0Or1) {
    Nfa nfa{"[a-c]?"};
    string s = "abc";
    auto begin = s.cbegin(), end = s.cend();

    auto match_end = nfa.NextMatch(begin, end)->first.second;
    EXPECT_EQ(string(begin, match_end), "a");

    begin = match_end;
    match_end = nfa.NextMatch(begin, end)->first.second;
    EXPECT_EQ(string(begin, match_end), "b");

    begin = match_end;
    match_end = nfa.NextMatch(begin, end)->first.second;
    EXPECT_EQ(string(begin, match_end), "c");

    begin = match_end;
    match_end = nfa.NextMatch(begin, end)->first.second;
    EXPECT_EQ(string(begin, match_end), "");
}

TEST(Nfa, Quantifier_0OrMore) {
    Nfa nfa{"[a-c]*"};
    string s = "abc";
    auto begin = s.cbegin(), end = s.cend();

    auto match_end = nfa.NextMatch(begin, end)->first.second;
    EXPECT_EQ(string(begin, match_end), "abc");

    begin = match_end;
    match_end = nfa.NextMatch(begin, end)->first.second;
    EXPECT_EQ(string(begin, match_end), "");
}

TEST(Nfa, Quantifier_1OrMore) {
    Nfa nfa{"[a-c]+"};
    string s = "abcd";
    auto begin = s.cbegin(), end = s.cend();

    auto match_end = nfa.NextMatch(begin, end)->first.second;
    EXPECT_EQ(string(begin, match_end), "abc");

    begin = match_end;
    EXPECT_EQ(nfa.NextMatch(begin, end), nullptr);
}

TEST(Nfa, Quantifier_Exact) {
    Nfa nfa{"[a-c]{2}"};
    string s = "abcd";
    auto begin = s.cbegin(), end = s.cend();

    auto match_end = nfa.NextMatch(begin, end)->first.second;
    EXPECT_EQ(string(begin, match_end), "ab");

    begin = match_end;
    EXPECT_EQ(nfa.NextMatch(begin, end), nullptr);
}

TEST(Nfa, Quantifier_nOrMore) {
    Nfa nfa{"[a-c]{2,}"};
    string s = "abcd";
    auto begin = s.cbegin(), end = s.cend();

    auto match_end = nfa.NextMatch(begin, end)->first.second;
    EXPECT_EQ(string(begin, match_end), "abc");

    begin = match_end;
    EXPECT_EQ(nfa.NextMatch(begin, end), nullptr);
}

TEST(Nfa, Quantifier_mTon) {
    Nfa nfa{"[a-c]{2,4}"};
    string s = "abcabd";
    auto begin = s.cbegin(), end = s.cend();

    auto match_end = nfa.NextMatch(begin, end)->first.second;
    EXPECT_EQ(string(begin, match_end), "abca");

    begin = match_end;
    EXPECT_EQ(nfa.NextMatch(begin, end), nullptr);
}

TEST(Nfa, Assertion_PositiveLookahead) {
    Nfa nfa{"(?=a)ab"};
    string s = "ab";
    auto begin = s.cbegin(), end = s.cend();

    auto match_end = nfa.NextMatch(begin, end)->first.second;
    EXPECT_EQ(string(begin, match_end), "ab");

    begin = match_end;
    EXPECT_EQ(nfa.NextMatch(begin, end), nullptr);
}

TEST(Nfa, Assertion_NegativeLookahead) {
    Nfa nfa{"(?!abd)abc"};
    string s = "abc";
    auto begin = s.cbegin(), end = s.cend();

    auto match_end = nfa.NextMatch(begin, end)->first.second;
    EXPECT_EQ(string(begin, match_end), "abc");

    begin = match_end;
    EXPECT_EQ(nfa.NextMatch(begin, end), nullptr);
}

TEST(Nfa, ContinuousAssertion) {
    Nfa nfa{"(?!ad)(?=ab)ab"};
    string s = "abab";
    auto begin = s.cbegin(), end = s.cend();

    auto match_end = nfa.NextMatch(begin, end)->first.second;
    EXPECT_EQ(string(begin, match_end), "ab");

    begin = match_end;
    match_end = nfa.NextMatch(begin, end)->first.second;
    EXPECT_EQ(string(begin, match_end), "ab");

    begin = match_end;
    EXPECT_EQ(nfa.NextMatch(begin, end), nullptr);
}

TEST(Nfa, Assertion_LineBegin) {
    Nfa nfa{"^a+"};
    string s = "aaa";
    auto begin = s.cbegin(), end = s.cend();

    auto match_end = nfa.NextMatch(begin, end)->first.second;
    EXPECT_EQ(string(begin, match_end), "aaa");

    begin = match_end;
    EXPECT_EQ(nfa.NextMatch(begin, end), nullptr);
}

TEST(Nfa, Assertion_LineEnd) {
    Nfa nfa{"a+$"};
    string s = "aaa";
    auto begin = s.cbegin(), end = s.cend();

    auto match_end = nfa.NextMatch(begin, end)->first.second;
    EXPECT_EQ(string(begin, match_end), "aaa");

    begin = match_end;
    EXPECT_EQ(nfa.NextMatch(begin, end), nullptr);
}

TEST(Nfa, Assertion_WordBoundary) {
    Nfa nfa{"\\ba+\\b"};
    string s = "aaa aaa";
    auto begin = s.cbegin(), end = s.cend();

    auto match_end = nfa.NextMatch(begin, end)->first.second;
    EXPECT_EQ(string(begin, match_end), "aaa");

    begin = match_end;
    EXPECT_EQ(nfa.NextMatch(begin, end), nullptr);
}

TEST(Nfa, Assertion_NotWordBoundary) {
    Nfa nfa{"aa\\Ba"};
    string s = "aaa";
    auto begin = s.cbegin(), end = s.cend();

    auto match_end = nfa.NextMatch(begin, end)->first.second;
    EXPECT_EQ(string(begin, match_end), "aaa");

    begin = match_end;
    EXPECT_EQ(nfa.NextMatch(begin, end), nullptr);
}

TEST(Nfa, Group) {
    Nfa nfa{"(aa)ab"};
    string s = "aaabc";
    auto begin = s.cbegin(), end = s.cend();

    auto match_end = nfa.NextMatch(begin, end)->first.second;
    EXPECT_EQ(string(begin, match_end), "aaab");

    begin = match_end;
    EXPECT_EQ(nfa.NextMatch(begin, end), nullptr);
}

TEST(Nfa, PassiveGroup) {
    Nfa nfa{"(?:abc)a"};
    string s = "abca";
    auto begin = s.cbegin(), end = s.cend();

    auto match_end = nfa.NextMatch(begin, end)->first.second;
    EXPECT_EQ(string(begin, match_end), "abca");

    begin = match_end;
    EXPECT_EQ(nfa.NextMatch(begin, end), nullptr);
}

TEST(Nfa, NestedGroup) {
    Nfa nfa{"(^aa(ab))c"};
    string s = "aaabc";
    auto begin = s.cbegin(), end = s.cend();

    auto match_end = nfa.NextMatch(begin, end)->first.second;
    EXPECT_EQ(string(begin, match_end), "aaabc");

    begin = match_end;
    EXPECT_EQ(nfa.NextMatch(begin, end), nullptr);
}

TEST(Nfa, BackReference) {
    Nfa nfa{"(a*)bc\\1"};
    string s = "aabcaaa";
    auto begin = s.cbegin(), end = s.cend();

    auto match_end = nfa.NextMatch(begin, end)->first.second;
    EXPECT_EQ(string(begin, match_end), "aabcaa");

    begin = match_end;
    EXPECT_EQ(nfa.NextMatch(begin, end), nullptr);
}

TEST(Nfa, SeveralBackReference) {
    Nfa nfa{R"((a*)(b*)c\1\1\2)"};
    string s = "aabcaaaab";
    auto begin = s.cbegin(), end = s.cend();

    auto match_end = nfa.NextMatch(begin, end)->first.second;
    EXPECT_EQ(string(begin, match_end), s);

    begin = match_end;
    EXPECT_EQ(nfa.NextMatch(begin, end), nullptr);
}

TEST(Nfa, NotNewLine) {
    Nfa nfa{"..."};
    string s = "aaa";
    auto begin = s.cbegin(), end = s.cend();

    auto match_end = nfa.NextMatch(begin, end)->first.second;
    EXPECT_EQ(string(begin, match_end), "aaa");

    begin = match_end;
    EXPECT_EQ(nfa.NextMatch(begin, end), nullptr);
}

TEST(Nfa, EscapeCharacter) {
    Nfa nfa{"\\(a+\\)"};
    string s = "(a)";
    auto begin = s.cbegin(), end = s.cend();

    auto match_end = nfa.NextMatch(begin, end)->first.second;
    EXPECT_EQ(string(begin, match_end), "(a)");

    begin = match_end;
    EXPECT_EQ(nfa.NextMatch(begin, end), nullptr);
}

TEST(Nfa, SpecialPatternInRange) {
    Nfa nfa{"[\\w]"};
    string s = "a1";
    auto begin = s.cbegin(), end = s.cend();

    auto match_end = nfa.NextMatch(begin, end)->first.second;
    EXPECT_EQ(string(begin, match_end), "a");

    begin = match_end;
    match_end = nfa.NextMatch(begin, end)->first.second;
    EXPECT_EQ(string(begin, match_end), "1");

    begin = match_end;
    EXPECT_EQ(nfa.NextMatch(begin, end), nullptr);
}

TEST(Nfa, ExceptRange) {
    Nfa nfa{"[^abc\\d]"};
    string s = "d";
    auto begin = s.cbegin(), end = s.cend();

    auto match_end = nfa.NextMatch(begin, end)->first.second;
    EXPECT_EQ(string(begin, match_end), "d");

    begin = match_end;
    EXPECT_EQ(nfa.NextMatch(begin, end), nullptr);
}