//
// Created by dxy on 2020/8/10.
//

#include "gtest/gtest.h"
#include "nfa.h"

using namespace XyRegEngine;
using namespace std;

TEST(Nfa, InvalidRegex) {
  Nfa<char> nfa("a|");

  EXPECT_TRUE(nfa.Empty());
}

TEST(Nfa, Alternative) {
  Nfa<char> nfa("a|b");
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
  Nfa<char> nfa("ab");
  string s = "abc";
  auto begin = s.cbegin(), end = s.cend();

  auto match_end = nfa.NextMatch(begin, end)->first.second;
  EXPECT_EQ(string(begin, match_end), "ab");

  begin = match_end;
  EXPECT_EQ(nfa.NextMatch(begin, end), nullptr);
}

TEST(Nfa, Range) {
  Nfa<char> nfa("[a-c]");
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
  Nfa<char> nfa("[a-c]?");
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
  Nfa<char> nfa("[a-c]*");
  string s = "abc";
  auto begin = s.cbegin(), end = s.cend();

  auto match_end = nfa.NextMatch(begin, end)->first.second;
  EXPECT_EQ(string(begin, match_end), "abc");

  begin = match_end;
  match_end = nfa.NextMatch(begin, end)->first.second;
  EXPECT_EQ(string(begin, match_end), "");
}

TEST(Nfa, Quantifier_1OrMore) {
  Nfa<char> nfa("[a-c]+");
  string s = "abcd";
  auto begin = s.cbegin(), end = s.cend();

  auto match_end = nfa.NextMatch(begin, end)->first.second;
  EXPECT_EQ(string(begin, match_end), "abc");

  begin = match_end;
  EXPECT_EQ(nfa.NextMatch(begin, end), nullptr);
}

TEST(Nfa, Quantifier_Exact) {
  Nfa<char> nfa("[a-c]{2}");
  string s = "abcd";
  auto begin = s.cbegin(), end = s.cend();

  auto match_end = nfa.NextMatch(begin, end)->first.second;
  EXPECT_EQ(string(begin, match_end), "ab");

  begin = match_end;
  EXPECT_EQ(nfa.NextMatch(begin, end), nullptr);
}

TEST(Nfa, Quantifier_nOrMore) {
  Nfa<char> nfa("[a-c]{2,}");
  string s = "abcd";
  auto begin = s.cbegin(), end = s.cend();

  auto match_end = nfa.NextMatch(begin, end)->first.second;
  EXPECT_EQ(string(begin, match_end), "abc");

  begin = match_end;
  EXPECT_EQ(nfa.NextMatch(begin, end), nullptr);
}

TEST(Nfa, Quantifier_mTon) {
  Nfa<char> nfa("[a-c]{2,4}");
  string s = "abcabd";
  auto begin = s.cbegin(), end = s.cend();

  auto match_end = nfa.NextMatch(begin, end)->first.second;
  EXPECT_EQ(string(begin, match_end), "abca");

  begin = match_end;
  EXPECT_EQ(nfa.NextMatch(begin, end), nullptr);
}

TEST(Nfa, Assertion_PositiveLookahead) {
  Nfa<char> nfa("(?=a)ab");
  string s = "ab";
  auto begin = s.cbegin(), end = s.cend();

  auto match_end = nfa.NextMatch(begin, end)->first.second;
  EXPECT_EQ(string(begin, match_end), "ab");

  begin = match_end;
  EXPECT_EQ(nfa.NextMatch(begin, end), nullptr);
}

TEST(Nfa, Assertion_NegativeLookahead) {
  Nfa<char> nfa("(?!abd)abc");
  string s = "abc";
  auto begin = s.cbegin(), end = s.cend();

  auto match_end = nfa.NextMatch(begin, end)->first.second;
  EXPECT_EQ(string(begin, match_end), "abc");

  begin = match_end;
  EXPECT_EQ(nfa.NextMatch(begin, end), nullptr);
}

TEST(Nfa, ContinuousAssertion) {
  Nfa<char> nfa("(?!ad)(?=ab)ab");
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
  Nfa<char> nfa("^a+");
  string s = "aaa";
  auto begin = s.cbegin(), end = s.cend();

  auto match_end = nfa.NextMatch(begin, end)->first.second;
  EXPECT_EQ(string(begin, match_end), "aaa");

  begin = match_end;
  EXPECT_EQ(nfa.NextMatch(begin, end), nullptr);
}

TEST(Nfa, Assertion_LineEnd) {
  Nfa<char> nfa("a+$");
  string s = "aaa";
  auto begin = s.cbegin(), end = s.cend();

  auto match_end = nfa.NextMatch(begin, end)->first.second;
  EXPECT_EQ(string(begin, match_end), "aaa");

  begin = match_end;
  EXPECT_EQ(nfa.NextMatch(begin, end), nullptr);
}

TEST(Nfa, Assertion_WordBoundary) {
  Nfa<char> nfa("\\ba+\\b");
  string s = "aaa aaa";
  auto begin = s.cbegin(), end = s.cend();

  auto match_end = nfa.NextMatch(begin, end)->first.second;
  EXPECT_EQ(string(begin, match_end), "aaa");

  begin = match_end;
  EXPECT_EQ(nfa.NextMatch(begin, end), nullptr);
}

TEST(Nfa, Assertion_NotWordBoundary) {
  Nfa<char> nfa("aa\\Ba");
  string s = "aaa";
  auto begin = s.cbegin(), end = s.cend();

  auto match_end = nfa.NextMatch(begin, end)->first.second;
  EXPECT_EQ(string(begin, match_end), "aaa");

  begin = match_end;
  EXPECT_EQ(nfa.NextMatch(begin, end), nullptr);
}

TEST(Nfa, Group) {
  Nfa<char> nfa("(aa)ab");
  string s = "aaabc";
  auto begin = s.cbegin(), end = s.cend();

  auto match_end = nfa.NextMatch(begin, end)->first.second;
  EXPECT_EQ(string(begin, match_end), "aaab");

  begin = match_end;
  EXPECT_EQ(nfa.NextMatch(begin, end), nullptr);
}

TEST(Nfa, PassiveGroup) {
  Nfa<char> nfa("(?:abc)a");
  string s = "abca";
  auto begin = s.cbegin(), end = s.cend();

  auto match_end = nfa.NextMatch(begin, end)->first.second;
  EXPECT_EQ(string(begin, match_end), "abca");

  begin = match_end;
  EXPECT_EQ(nfa.NextMatch(begin, end), nullptr);
}

TEST(Nfa, NestedGroup) {
  Nfa<char> nfa("(^aa(ab))c");
  string s = "aaabc";
  auto begin = s.cbegin(), end = s.cend();

  auto match_end = nfa.NextMatch(begin, end)->first.second;
  EXPECT_EQ(string(begin, match_end), "aaabc");

  begin = match_end;
  EXPECT_EQ(nfa.NextMatch(begin, end), nullptr);
}

TEST(Nfa, BackReference) {
  Nfa<char> nfa("(a*)bc\\1");
  string s = "aabcaaa";
  auto begin = s.cbegin(), end = s.cend();

  auto match_end = nfa.NextMatch(begin, end)->first.second;
  EXPECT_EQ(string(begin, match_end), "aabcaa");

  begin = match_end;
  EXPECT_EQ(nfa.NextMatch(begin, end), nullptr);
}

TEST(Nfa, SeveralBackReference) {
  Nfa<char> nfa(R"((a*)(b*)c\1\1\2)");
  string s = "aabcaaaab";
  auto begin = s.cbegin(), end = s.cend();

  auto match_end = nfa.NextMatch(begin, end)->first.second;
  EXPECT_EQ(string(begin, match_end), s);

  begin = match_end;
  EXPECT_EQ(nfa.NextMatch(begin, end), nullptr);
}

TEST(Nfa, NotNewLine) {
  Nfa<char> nfa("...");
  string s = "aaa";
  auto begin = s.cbegin(), end = s.cend();

  auto match_end = nfa.NextMatch(begin, end)->first.second;
  EXPECT_EQ(string(begin, match_end), "aaa");

  begin = match_end;
  EXPECT_EQ(nfa.NextMatch(begin, end), nullptr);
}

TEST(Nfa, EscapeCharacter) {
  Nfa<char> nfa("\\(a+\\)");
  string s = "(a)";
  auto begin = s.cbegin(), end = s.cend();

  auto match_end = nfa.NextMatch(begin, end)->first.second;
  EXPECT_EQ(string(begin, match_end), "(a)");

  begin = match_end;
  EXPECT_EQ(nfa.NextMatch(begin, end), nullptr);
}

TEST(Nfa, SpecialPatternInRange) {
  Nfa<char> nfa("[\\w]");
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
  Nfa<char> nfa("[^abc\\d]");
  string s = "d";
  auto begin = s.cbegin(), end = s.cend();

  auto match_end = nfa.NextMatch(begin, end)->first.second;
  EXPECT_EQ(string(begin, match_end), "d");

  begin = match_end;
  EXPECT_EQ(nfa.NextMatch(begin, end), nullptr);
}

TEST(Nfa, UTF8) {
  Nfa<wchar_t> nfa(L"的");
  wstring s = L"的";
  auto begin = s.cbegin(), end = s.cend();

  auto match_end = nfa.NextMatch(begin, end)->first.second;
  EXPECT_EQ(wstring(begin, match_end), L"的");

  begin = match_end;
  EXPECT_EQ(nfa.NextMatch(begin, end), nullptr);
}