//
// Created by dxy on 2020/9/17.
//

#ifndef XYREGENGINE_XY_REGEX_H
#define XYREGENGINE_XY_REGEX_H

#include "nfa.h"

namespace XyRegEngine {
template<class T>
class Regex;

template<class T>
class RegexResult {
  friend class Regex<T>;

 public:
  [[nodiscard]] SubMatch<T> GetResult() const {
    return result_;
  }

  [[nodiscard]] std::vector<SubMatch<T>> GetSubMatches() const {
    return sub_matches_;
  }

 private:
  SubMatch<T> result_;  // store the string that matches the given regex
  // All sub-matches are stored in sequence.
  std::vector<SubMatch<T>> sub_matches_;
};

template<class T>
class Regex {
 public:
  explicit Regex(const std::basic_string<T> &regex) : nfa_(regex) {}

  /**
   * Determine whether s matches the regex.
   *
   * @param s
   * @param result store detailed result
   * @return
   */
  bool Match(const std::basic_string<T> &s, RegexResult<T> &result);

  /**
   * Determine whether a sub-string in s matches the regex.
   *
   * @param s
   * @param result store detailed result
   * @return
   */
  bool Search(const std::basic_string<T> &s, RegexResult<T> &result);

 private:
  Nfa<T> nfa_;
};

template<class T>
bool Regex<T>::Match(const std::basic_string<T> &s, RegexResult<T> &result) {
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

template<class T>
bool Regex<T>::Search(const std::basic_string<T> &s, RegexResult<T> &result) {
  auto begin = s.cbegin(), end = s.cend();

  while (begin != end) {
    auto state_ptr = nfa_.NextMatch(begin, end);
    if (state_ptr != nullptr) {
      result.result_ = {begin, state_ptr->first.second};
      for (const auto &sub_match:state_ptr->second) {
        result.sub_matches_.emplace_back(sub_match.first, sub_match.second);
      }

      return true;
    }
    begin++;
  }

  return false;
}
}

#endif //XYREGENGINE_XY_REGEX_H