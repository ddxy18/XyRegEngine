//
// Created by dxy on 2020/9/17.
//

#include "xy_regex.h"

using namespace XyRegEngine;
using namespace std;

bool Regex::Match(const string &s, RegexResult &result) {
    auto state_ptr = nfa_.NextMatch(s.cbegin(), s.cend());

    if (state_ptr == nullptr || state_ptr->first.second != s.cend()) {
        return false;
    }

    // set result
    result.result_ = {s.cbegin(), s.cend()};
    for (const auto &sub_match:state_ptr->second) {
        result.sub_matches_.emplace_back(sub_match.first, sub_match.second);
    }
    return true;
}

bool Regex::Search(const string &s, RegexResult &result) {
    auto begin = s.cbegin(), end = s.cend();

    while (begin != end) {
        auto state_ptr = nfa_.NextMatch(begin, end);
        if (state_ptr != nullptr) {
            result.result_ = {begin, state_ptr->first.second};
            for (const auto &sub_match:state_ptr->second) {
                result.sub_matches_.emplace_back(sub_match.first,
                                                 sub_match.second);
            }

            return true;
        }
        begin++;
    }

    return false;
}
