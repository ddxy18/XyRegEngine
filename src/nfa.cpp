//
// Created by dxy on 2020/10/18.
//

#include "nfa.h"

using namespace XyRegEngine;

template<>
AssertionNfa<char>::AssertionNfa(const std::basic_string<char> &assertion) {
  using namespace std;

  if (assertion[0] != '(') {
    if (assertion == "^") {
      type_ = AssertionType::kLineBegin;
    } else if (assertion == "$") {
      type_ = AssertionType::kLineEnd;
    } else if (assertion == "\\b") {
      type_ = AssertionType::kWordBoundary;
    } else if (assertion == "\\B") {
      type_ = AssertionType::kNotWordBoundary;
    }
  } else {  // lookahead
    if (assertion[2] == '=') {
      type_ = AssertionType::kPositiveLookahead;
    } else {
      type_ = AssertionType::kNegativeLookahead;
    }
    string regex{assertion.cbegin() + 3, assertion.cend() - 1};
    nfa_ = Nfa<char>{regex};
  }
}

template<>
AssertionNfa<wchar_t>::AssertionNfa(
        const std::basic_string<wchar_t> &assertion) {
  using namespace std;

  if (assertion[0] != '(') {
    if (assertion == L"^") {
      type_ = AssertionType::kLineBegin;
    } else if (assertion == L"$") {
      type_ = AssertionType::kLineEnd;
    } else if (assertion == L"\\b") {
      type_ = AssertionType::kWordBoundary;
    } else if (assertion == L"\\B") {
      type_ = AssertionType::kNotWordBoundary;
    }
  } else {  // lookahead
    if (assertion[2] == '=') {
      type_ = AssertionType::kPositiveLookahead;
    } else {
      type_ = AssertionType::kNegativeLookahead;
    }
    wstring regex{assertion.cbegin() + 3, assertion.cend() - 1};
    nfa_ = Nfa<wchar_t>{regex};
  }
}

template<>
StrConstIt<char> SpecialPatternNfa<char>::NextMatch(const State<char> &state,
                                                    StrConstIt<char> str_end) {
  using namespace std;

  auto begin = state.first.second;

  if (begin<str_end) {
    if (characters_ == ".") {  // not new line
      if (*begin != '\n' && *begin != '\r') {
        return begin + 1;
      }
    } else if (characters_ == "\\d") {  // digit
      if (isdigit(*begin)) {
        return begin + 1;
      }
    } else if (characters_ == "\\D") {  // not digit
      if (!isdigit(*begin)) {
        return begin + 1;
      }
    } else if (characters_ == "\\s") {  // whitespace
      if (isspace(*begin)) {
        return begin + 1;
      }
    } else if (characters_ == "\\S") {  // not whitespace
      if (!isspace(*begin)) {
        return begin + 1;
      }
    } else if (characters_ == "\\w") {  // word
      if (isalnum(*begin)) {
        return begin + 1;
      }
    } else if (characters_ == "\\W") {  // not word
      if (!isalnum(*begin)) {
        return begin + 1;
      }
    } else if (characters_ == "\\t") {  // \t
      if (*begin == '\t') {
        return begin + 1;
      }
    } else if (characters_ == "\\n") {  // \n
      if (*begin == '\n') {
        return begin + 1;
      }
    } else if (characters_ == "\\v") {  // \v
      if (*begin == '\v') {
        return begin + 1;
      }
    } else if (characters_ == "\\f") {  // \f
      if (*begin == '\f') {
        return begin + 1;
      }
    } else if (characters_ == "\\0") {  // \0
      if (*begin == '\0') {
        return begin + 1;
      }
    } else if (isdigit(characters_[1])) {  // back reference
      int back_reference = stoi(
              string{characters_.cbegin() + 1, characters_.cend()});
      int length = state.second[back_reference - 1].second -
                   state.second[back_reference - 1].first;
      for (int i = 0; i < length; ++i) {
        if (*(begin + i) !=
            *(state.second[back_reference - 1].first +
              i)) {
          return begin;
        }
      }
      return begin + length;
    } else {  // \\^ \\$ \\\\ \\. \\* \\+ \\? \\( \\) \\[ \\] \\{ \\} \\|
      if (*begin == characters_[1]) {
        return begin + 1;
      }
    }
  }
  // TODO(dxy): \\c \\x \\u

  return begin;
}

template<>
StrConstIt<wchar_t>
SpecialPatternNfa<wchar_t>::NextMatch(const State<wchar_t> &state,
                                      StrConstIt<wchar_t> str_end) {
  using namespace std;

  auto begin = state.first.second;

  if (begin<str_end) {
    if (characters_ == L".") {  // not new line
      if (*begin != '\n' && *begin != '\r') {
        return begin + 1;
      }
    } else if (characters_ == L"\\d") {  // digit
      if (isdigit(*begin)) {
        return begin + 1;
      }
    } else if (characters_ == L"\\D") {  // not digit
      if (!isdigit(*begin)) {
        return begin + 1;
      }
    } else if (characters_ == L"\\s") {  // whitespace
      if (isspace(*begin)) {
        return begin + 1;
      }
    } else if (characters_ == L"\\S") {  // not whitespace
      if (!isspace(*begin)) {
        return begin + 1;
      }
    } else if (characters_ == L"\\w") {  // word
      if (isalnum(*begin)) {
        return begin + 1;
      }
    } else if (characters_ == L"\\W") {  // not word
      if (!isalnum(*begin)) {
        return begin + 1;
      }
    } else if (characters_ == L"\\t") {  // \t
      if (*begin == '\t') {
        return begin + 1;
      }
    } else if (characters_ == L"\\n") {  // \n
      if (*begin == '\n') {
        return begin + 1;
      }
    } else if (characters_ == L"\\v") {  // \v
      if (*begin == '\v') {
        return begin + 1;
      }
    } else if (characters_ == L"\\f") {  // \f
      if (*begin == '\f') {
        return begin + 1;
      }
    } else if (characters_ == L"\\0") {  // \0
      if (*begin == '\0') {
        return begin + 1;
      }
    } else if (isdigit(characters_[1])) {  // back reference
      int back_reference = stoi(
              wstring{characters_.cbegin() + 1, characters_.cend()});
      int length = state.second[back_reference - 1].second -
                   state.second[back_reference - 1].first;
      for (int i = 0; i < length; ++i) {
        if (*(begin + i) !=
            *(state.second[back_reference - 1].first +
              i)) {
          return begin;
        }
      }
      return begin + length;
    } else {  // \\^ \\$ \\\\ \\. \\* \\+ \\? \\( \\) \\[ \\] \\{ \\} \\|
      if (*begin == characters_[1]) {
        return begin + 1;
      }
    }
  }
  // TODO(dxy): \\c \\x \\u

  return begin;
}