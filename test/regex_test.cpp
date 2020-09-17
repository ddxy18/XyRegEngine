//
// Created by dxy on 2020/9/17.
//

#include "gtest/gtest.h"
#include "xy_regex.h"

using namespace XyRegEngine;
using namespace std;

TEST(Regex, MatchSuccess) {
    Regex regex{"a|b"};
    RegexResult result;

    EXPECT_TRUE(regex.Match("a", result));

    SubMatch sub_match = result.GetResult();
    EXPECT_EQ(string(sub_match.first, sub_match.second), "a");

    EXPECT_TRUE(result.GetSubMatches().empty());
}

TEST(Regex, MatchFailure) {
    Regex regex{"a|b"};
    RegexResult result;

    EXPECT_FALSE(regex.Match("ab", result));
    EXPECT_TRUE(result.GetSubMatches().empty());
}

TEST(Regex, SearchSuccess) {
    Regex regex{"(a*)ab\\1"};
    RegexResult result;

    EXPECT_TRUE(regex.Search("ccaabaaa", result));

    SubMatch sub_match = result.GetResult();
    auto s = string{sub_match.first, sub_match.second};
    EXPECT_EQ(s, "aaba");

    sub_match = result.GetSubMatches()[0];
    EXPECT_EQ(string(sub_match.first, sub_match.second), "a");
}

TEST(Regex, SearchFailure) {
    Regex regex{"(a*)ab\\1"};
    RegexResult result;

    EXPECT_FALSE(regex.Search("ccbaaa", result));
    EXPECT_TRUE(result.GetSubMatches().empty());
}