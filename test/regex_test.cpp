//
// Created by dxy on 2020/9/17.
//

#include "gtest/gtest.h"
#include "xy_regex.h"

#include <codecvt>

using namespace XyRegEngine;
using namespace std;

TEST(Regex, MatchSuccess) {
  Regex<char> regex("a|b");
  RegexResult<char> result;

  EXPECT_TRUE(regex.Match("a", result));

  auto sub_match = result.GetResult();
  EXPECT_EQ(string(sub_match.first, sub_match.second), "a");

  EXPECT_TRUE(result.GetSubMatches().empty());
}

TEST(Regex, MatchFailure) {
  Regex<char> regex("a|b");
  RegexResult<char> result;

  EXPECT_FALSE(regex.Match("ab", result));
  EXPECT_TRUE(result.GetSubMatches().empty());
}

TEST(Regex, SearchSuccess) {
  Regex<char> regex("(a*)ab\\1");
  RegexResult<char> result;

  EXPECT_TRUE(regex.Search("ccaabaaa", result));

  auto sub_match = result.GetResult();
  auto s = string(sub_match.first, sub_match.second);
  EXPECT_EQ(s, "aaba");

  sub_match = result.GetSubMatches()[0];
  EXPECT_EQ(string(sub_match.first, sub_match.second), "a");
}

TEST(Regex, SearchFailure) {
  Regex<char> regex("(a*)ab\\1");
  RegexResult<char> result;

  EXPECT_FALSE(regex.Search("ccbaaa", result));
  EXPECT_TRUE(result.GetSubMatches().empty());
}

TEST(Regex, UTF8) {
  Regex<wchar_t> regex(L"(?:0|的)+");
  RegexResult<wchar_t> result;

  EXPECT_TRUE(regex.Search(L"1的0", result));

  auto sub_match = result.GetResult();
  auto s = wstring(sub_match.first, sub_match.second);
  EXPECT_EQ(s, L"的0");

  EXPECT_TRUE(regex.Search(L"10的", result));

  sub_match = result.GetResult();
  s = wstring(sub_match.first, sub_match.second);
  EXPECT_EQ(s, L"0的");
}