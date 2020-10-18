//
// Created by dxy on 2020/8/6.
//

#ifndef XYREGENGINE_NFA_H
#define XYREGENGINE_NFA_H

#include <climits>
#include <map>
#include <memory>
#include <stack>
#include <set>
#include <string>
#include <vector>

#include "lex.h"

namespace XyRegEngine {
template<class T>
class AstNode;

template<class T>
class NfaFactory;

template<class T>
class AssertionNfa;

template<class T>
class GroupNfa;

template<class T>
class SpecialPatternNfa;

template<class T>
class RangeNfa;

template<class T>
using AstNodePtr = std::unique_ptr<AstNode<T>>;
// a sub-match [pair.first, pair.second)
template<class T> using SubMatch = std::pair<StrConstIt<T>, StrConstIt<T>>;
// pair.first -- state
// pair.second -- current begin iterator
// map.second -- Sub-matches. Every vector<SubMatch> stores a possible
// sub-match order.
template<class T> using ReachableStatesMap = std::map<std::pair<int, StrConstIt<T>>, std::vector<SubMatch<T>>>;
template<class T> using State = std::pair<std::pair<int, StrConstIt<T>>, std::vector<SubMatch<T>>>;
template<class T> using StatePtr = std::unique_ptr<State<T>>;

enum class Encoding {
  kAscii, kUtf8
};

/**
 * We split a regex to several parts and classify them to types in
 * RegexPart.
 *
 * kAnd('&': it is implicit in the regex)
 * kChar(including [...], escape character and single character)
 * kQuantifier('*' '?' '+' and their non-greedy mode, '{...}')
 * kAlternative('|')
 * kCapture('(...)' or '(?:...)')
 * kAssertion('(?=...)', '(?!...)', '^', '$', '\b', '\B')
 * kError(invalid token)
 */
enum class RegexPart {
  kAnd, kChar, kQuantifier, kAlternative, kGroup, kAssertion, kError
};

template<class T>
class Nfa {
  friend class NfaFactory<T>;

  friend class AssertionNfa<T>;

 public:
  /**
   * Build a NFA for 'regex'. Notice that if 'regex' is invalid, it
   * creates an empty NFA.
   *
   * @param regex
   */
  explicit Nfa(const std::basic_string<T> &regex);

  /**
   * It is used to determine whether a NFA is a valid NFA.
   *
   * @return
   */
  [[nodiscard]] bool Empty() const {
    // all invalid NFAs should have accept_state_ set as -1
    return accept_state_ == -1;
  }

  /**
   * Get the next match in a given string in the range of [begin, end).
   * Notice that it matches from begin, say, the successful match must
   * be [begin, any location no more than end).
   *
   * @param begin First iterator of the given string. If we successfully
   * find a token, 'begin' will be moved to the beginning of the first
   * character after the match.
   * @param end Last iterator of the given string.
   * @return A matched substring. If no match exists, it returns "".
   */
  StatePtr<T> NextMatch(StrConstIt<T> begin, StrConstIt<T> end);

 protected:
  enum class StateType {
    kAssertion, kGroup, kSpecialPattern, kRange, kCommon
  };

  static const int kEmptyEdge = 0;

  Nfa() = default;

  /**
   * Build a NFA according to an AST. If ast_head is an invalid AST or
   * a nullptr pointer, it creates an empty NFA.
   *
   * @param ast_head can be nullptr
   */
  Nfa(AstNodePtr<T> &ast_head, const std::vector<unsigned int> &char_ranges);

  /**
   * It copies exchange_map_, assertion_states_, not_assertion_states_
   * and group_states_ to the new NFA.
   *
   * @param nfa After calling the function, nfa.exchange_map_ becomes
   * undefined and should never be accessed again.
   * @return
   */
  Nfa &operator+=(Nfa &nfa);

  /**
   * It tries to move to the next state until to the end of the string or
   * no next states can be found.
   *
   * @param begin
   * @param end
   * @return all possible routines
   */
  std::vector<ReachableStatesMap<T>>
  StateRoute(StrConstIt<T> begin, StrConstIt<T> end);

  /**
   * Get all reachable states starting from cur_state. When cur_state
   * is a functional state, it will be handled first. For common
   * states, we match *begin first. Then we deal with states that can
   * be reached through empty edges.
   *
   * @param cur_state
   * @param str_begin
   * @param str_end
   * @return reachable states after handling cur_state
   */
  ReachableStatesMap<T>
  NextState(const State<T> &cur_state,
            StrConstIt<T> str_begin, StrConstIt<T> str_end);

  /**
   * All reachable states starting from cur_state through empty edges.
   * Notice that cur_state is excluded and it does nothing to handle
   * functional states.
   *
   * @param cur_state
   * @return
   */
  ReachableStatesMap<T> NextState(const State<T> &cur_state);

  StateType GetStateType(int state);

  /**
   * Use 'delim' to split an encoding to several ranges. By default,
   * every range in an alphabet table is a character.
   *
   * @param delim You should ensure it contains valid character classes.
   * An empty vector means default split mode.
   * @param encoding
   */
  void CharRangesInit(const std::set<std::basic_string<T>> &delim,
                      Encoding encoding);

  /**
   * Find which character range in the char_ranges_ c is in.
   *
   * @param c must be in the range of the current encoding
   * @return range index in char_ranges_ where c is in
   */
  int GetCharLocation(int c);

  /**
   * Parse a regex to an AST. We assume that regex can only include
   * several valid elements:
   * characters(include single character, escape character and '[...]')
   * quantifiers
   * |
   * & (It is implicit in a regex string but we need to explicitly deal
   * with them when parsing a regex)
   * groups
   * assertions
   *
   * All characters should be represented as a node, say, ranges and
   * escape characters are seen as single character here. Moreover, groups
   * and lookaheads will add a flag node to the head to remember its type.
   *
   * @param regex
   * @return If regex is in a valid format, return the head of AST.
   * Otherwise return nullptr.
   */
  static AstNodePtr<T> ParseRegex(const std::basic_string<T> &regex);

  /**
   * Add a new state to exchange_map_. Now it has no edges.
   */
  void NewState();

  /**
   * We use i_ to generate a new state. First state should have a id 1.
   * After that, it will be increased when creating a new state. i_
   * points to the lastly added state at any time after creating the
   * first state.
   */
  static int i_;

  /**
   * Record several continuous character ranges. Ranges are stored
   * orderly. Range i refers to [char_ranges_[i], char_ranges_[i + 1]).
   * Since characters with code of 0-4 are almost never appears in
   * text, we use them to represent special edges. And char_ranges_
   * should coverage every character in the given encoding.
   */
  std::vector<unsigned int> char_ranges_;

  /**
   * map.first -- state
   * map.second.element -- reachable states from an input character that
   * is in the corresponding range
   */
  std::map<int, std::vector<std::set<int>>> exchange_map_;

  /**
   * We compress an assertion to an assertion state in the NFA and it is
   * connected to other states through empty edges. So we need a map to
   * connect the assertion state to the corresponding assertion NFA.
   */
  std::map<int, AssertionNfa<T>> assertion_states_;

  std::map<int, GroupNfa<T>> group_states_;

  std::map<int, SpecialPatternNfa<T>> special_pattern_states_;

  std::map<int, RangeNfa<T>> range_states_;

  int begin_state_{-1};
  int accept_state_{-1};
};

/**
 * ^ $ \\b \\B (?=sub-pattern) (?!sub-pattern)
 */
template<class T>
class AssertionNfa {
 public:
  AssertionNfa(const AssertionNfa &assertion_nfa) = default;

  explicit AssertionNfa(const std::basic_string<T> &assertion);

  /**
   * @param str_begin location of ^ in regex in a match
   * @param str_end location of $ in regex in a match
   * @param begin where to start the assertion
   * @return
   */
  bool IsSuccess(StrConstIt<T> str_begin, StrConstIt<T> str_end,
                 StrConstIt<T> begin);

 private:
  enum class AssertionType {
    kLineBegin, kLineEnd, kWordBoundary, kNotWordBoundary,
    kPositiveLookahead, kNegativeLookahead
  };

  AssertionType type_;

  Nfa<T> nfa_;
};

/**
 * It stores a NFA for the sub-pattern in the group.
 */
template<class T>
class GroupNfa : public Nfa<T> {
 public:
  GroupNfa(const GroupNfa &group_nfa) = default;

  explicit GroupNfa(const std::basic_string<T> &regex) : Nfa<T>(regex) {}

  /**
   * Get all possible sub-matches from the group.
   *
   * @param begin
   * @param str_end
   * @return possible end iterators after dealing with the group
   */
  std::set<StrConstIt<T>> NextMatch(StrConstIt<T> begin, StrConstIt<T> str_end);
};

/**
 * Escape characters, special meaning escape characters and back-reference.
 */
template<class T>
class SpecialPatternNfa {
 public:
  explicit SpecialPatternNfa(std::basic_string<T> characters)
          : characters_(std::move(characters)) {}

  /**
   * Determine whether a substring can match characters_.
   *
   * @param begin
   * @param str_end
   * @return If a substring [begin, end_it) matches, return end_it.
   * Otherwise return begin.
   */
  StrConstIt<T> NextMatch(const State<T> &state, StrConstIt<T> str_end);

 private:
  std::basic_string<T> characters_;
};

/**
 * [...]. It can contain single characters, ranges and special patterns.
 */
template<class T>
class RangeNfa {
 public:
  explicit RangeNfa(const std::basic_string<T> &regex);

  /**
   * @param begin
   * @param str_end
   * @return If a substring [begin, end_it) matches, return end_it.
   * Otherwise return begin.
   */
  StrConstIt<T> NextMatch(const State<T> &state, StrConstIt<T> str_end);

 private:
  std::map<int, int> ranges_;
  std::vector<SpecialPatternNfa<T>> special_patterns_;
  bool except_;  // true for [^...] and false for [...]
};

/**
 * Provide a function to create a sub-NFA for common operators in the
 * RegexPart. It's usually used to create NFAs for split parts.
 */
template<class T>
class NfaFactory {
 public:
  static Nfa<T> MakeCharacterNfa(const std::basic_string<T> &characters,
                                 const std::vector<unsigned int> &char_ranges);

  static Nfa<T> MakeAlternativeNfa(Nfa<T> left_nfa, Nfa<T> right_nfa);

  static Nfa<T> MakeAndNfa(Nfa<T> left_nfa, Nfa<T> right_nfa);

  static Nfa<T>
  MakeQuantifierNfa(const std::basic_string<T> &quantifier, AstNodePtr<T> &left,
                    const std::vector<unsigned int> &char_ranges);

 private:
  static std::pair<int, int>
  ParseQuantifier(const std::basic_string<T> &quantifier);
};

/**
 * Node in AST that is used as a intermediate format of a regex. It helps
 * to show the regex structure more clearly and reduce the work to build
 * the NFA. AST for regex is designed as a binary tree, so every AstNode
 * has at most two son nodes.
 */
template<class T>
class AstNode {
  friend class Nfa<T>;

 public:
  AstNode(RegexPart regex_type, std::basic_string<T> regex)
          : regex_type_(regex_type),
            regex_(std::move(regex)) {}

  void SetLeftSon(AstNodePtr<T> left_son) {
    left_son_ = std::move(left_son);
  }

  void SetRightSon(AstNodePtr<T> right_son) {
    right_son_ = std::move(right_son);
  }

  [[nodiscard]] RegexPart GetType() const {
    return regex_type_;
  }

 private:
  RegexPart regex_type_;
  std::basic_string<T> regex_;
  AstNodePtr<T> left_son_;
  AstNodePtr<T> right_son_;
};

template<class T> int Nfa<T>::i_ = 0;

/**
* It determines which RegexPart should be chosen according to regex's a few
* characters from its head.
*
* @param regex
* @return
*/
template<class T>
RegexPart GetRegexType(const std::basic_string<T> &regex);

template<class T>
std::set<std::basic_string<T>> GetDelim(const std::basic_string<T> &regex);

/**
 * Add a character range [begin, end) to char_ranges.
 *
 * @param begin
 * @param end
 */
void AddCharRange(std::set<unsigned int> &char_ranges,
                  unsigned int begin, unsigned int end);

/**
 * Add a char range [begin, begin + 1) to char_ranges.
 *
 * @param begin
 */
void AddCharRange(std::set<unsigned int> &char_ranges, unsigned int begin);

/**
* It finds one token a time and sets 'begin' to the head of the next token.
* Notice that '(...)' '{...}' '<...>' '[...]' are seen as a token.
*
* @param begin Always point to the begin of the next token. If no valid
* token remains, it points to 'iterator.cend()'.
* @param end Always point to the end of the given regex.
* @return A valid token. If it finds an invalid token or reaching to the end
 * of the regex, it returns "".
*/
template<class T>
std::basic_string<T> NextToken(StrConstIt<T> &begin, StrConstIt<T> &end);

/**
 * @param begin
 * @param end
 * @return Return the iterator of the first character after the escape
 * characters. If no valid escape characters are found, return begin.
 */
template<class T>
StrConstIt<T> SkipEscapeCharacters(StrConstIt<T> begin, StrConstIt<T> end);

template<class T>
bool PushAnd(std::stack<AstNodePtr<T>> &op_stack,
             std::stack<AstNodePtr<T>> &rpn_stack);

template<class T>
bool
PushOr(std::stack<AstNodePtr<T>> &op_stack,
       std::stack<AstNodePtr<T>> &rpn_stack);

template<class T>
bool PushQuantifier(std::stack<AstNodePtr<T>> &op_stack,
                    std::stack<AstNodePtr<T>> &rpn_stack,
                    const std::basic_string<T> &regex);

template<class T>
bool IsLineTerminator(StrConstIt<T> it);

template<class T>
bool IsWord(StrConstIt<T> it);

template<class T>
StatePtr<T> Nfa<T>::NextMatch(StrConstIt<T> begin, StrConstIt<T> end) {
  using namespace std;

  vector<ReachableStatesMap<T>> state_vec = StateRoute(begin, end);
  auto it = state_vec.cbegin();
  State<T> state = *state_vec[0].find({begin_state_, begin});

  // find the longest match
  while (it != state_vec.cend()) {
    for (const auto &cur_state:*it) {
      if (cur_state.first.first == accept_state_ &&
          cur_state.first.second >= state.first.second) {
        state = cur_state;
      }
    }
    it++;
  }

  if (state.first.first == begin_state_) {
    return nullptr;
  } else {
    return make_unique<State<T>>(state);
  }
}

template<class T>
std::vector<ReachableStatesMap<T>>
Nfa<T>::StateRoute(StrConstIt<T> begin, StrConstIt<T> end) {
  using namespace std;

  vector<ReachableStatesMap<T>> state_vec;
  ReachableStatesMap<T> cur_states;

  // Use states that can be accessed through empty edges from begin_state_
  // and begin_state_ itself as an initial driver.
  State<T> begin_state = {{begin_state_, begin}, vector<SubMatch<T>>()};
  if (GetStateType(begin_state_) == StateType::kCommon) {
    cur_states.merge(NextState(begin_state));
  }
  cur_states.insert(begin_state);
  state_vec.push_back(cur_states);

  // find all reachable states from current states
  while (!state_vec[state_vec.size() - 1].empty()) {
    cur_states.clear();
    for (const auto &last_state:state_vec[state_vec.size() - 1]) {
      cur_states.merge(NextState(last_state, begin, end));
    }
    state_vec.push_back(cur_states);
  }
  // remove the last empty vector
  state_vec.pop_back();

  return state_vec;
}

template<class T>
ReachableStatesMap<T>
Nfa<T>::NextState(const State<T> &cur_state,
                  StrConstIt<T> str_begin, StrConstIt<T> str_end) {
  ReachableStatesMap<T> next_states;
  auto begin = cur_state.first.second;

  switch (GetStateType(cur_state.first.first)) {
    case StateType::kAssertion:
      if (!assertion_states_.find(cur_state.first.first)->second.IsSuccess(
              str_begin, str_end, begin)) {
        return next_states;
      }
      next_states.insert(cur_state);
      break;
    case StateType::kGroup:
      for (auto end_it:
              group_states_.find(cur_state.first.first)->second.NextMatch(
                      begin, str_end)) {
        auto sub_matches = cur_state.second;
        sub_matches.emplace_back(begin, end_it);
        next_states.insert({{cur_state.first.first, end_it}, sub_matches});
      }
      break;
    case StateType::kSpecialPattern:
      begin = special_pattern_states_.find(
              cur_state.first.first)->second.NextMatch(cur_state, str_end);
      if (begin != cur_state.first.second) {
        next_states.insert({{cur_state.first.first, begin}, cur_state.second});
      }
      break;
    case StateType::kRange:
      begin = range_states_.find(cur_state.first.first)->second.NextMatch(
              cur_state, str_end);
      if (begin != cur_state.first.second) {
        next_states.insert({{cur_state.first.first, begin}, cur_state.second});
      }
      break;
    case StateType::kCommon:
      // add states that can be reached through *cur_it
      if (begin < str_end) {
        for (auto state:exchange_map_[cur_state.first.first][
                GetCharLocation(*begin)]) {
          next_states.insert({{state, begin + 1}, cur_state.second});
        }
      }
      break;
  }

  ReachableStatesMap<T> next_states_from_empty;
  for (const auto &state:next_states) {
    next_states_from_empty.merge(NextState(state));
  }
  next_states.erase({cur_state.first.first, begin});
  next_states.merge(next_states_from_empty);

  return next_states;
}

template<class T>
ReachableStatesMap<T> Nfa<T>::NextState(const State<T> &cur_state) {
  using namespace std;

  vector<int> common_states;
  set<int> func_states;

  common_states.push_back(cur_state.first.first);
  int i = -1;
  while (++i != common_states.size()) {
    for (auto state:exchange_map_[common_states[i]][kEmptyEdge]) {
      if (GetStateType(state) == StateType::kCommon) {
        if (find(common_states.cbegin(), common_states.cend(), state) ==
            common_states.cend()) {
          common_states.push_back(state);
        }
      } else {
        func_states.insert(state);
      }
    }
  }
  // erase cur_state
  common_states.erase(common_states.cbegin());

  ReachableStatesMap<T> next_states_map;
  for (auto state:common_states) {
    next_states_map.insert({{state, cur_state.first.second}, cur_state.second});
  }
  for (auto state:func_states) {
    next_states_map.insert({{state, cur_state.first.second}, cur_state.second});
  }

  return next_states_map;
}

template<class T>
typename Nfa<T>::StateType Nfa<T>::GetStateType(int state) {
  if (assertion_states_.contains(state)) {
    return StateType::kAssertion;
  }
  if (group_states_.contains(state)) {
    return StateType::kGroup;
  }
  if (special_pattern_states_.contains(state)) {
    return StateType::kSpecialPattern;
  }
  if (range_states_.contains(state)) {
    return StateType::kRange;
  }
  return StateType::kCommon;
}

template<class T>
std::set<std::basic_string<T>> GetDelim(const std::basic_string<T> &regex) {
  using namespace std;

  StrConstIt<T> begin = regex.cbegin(), end = regex.cend();
  set<basic_string<T>> delim;
  basic_string<T> token;

  while (!(token = NextToken<T>(begin, end)).empty()) {
    switch (GetRegexType(token)) {
      case RegexPart::kChar:
        delim.insert(token);
        break;
      case RegexPart::kGroup:
        if (regex[1] == '?') {  // passive group
          delim.merge(GetDelim(
                  basic_string<T>(token.cbegin() + 3, token.cend() - 1)));
        }
        break;
      default:
        break;
    }
  }

  return delim;
}

template<class T>
Nfa<T> &Nfa<T>::operator+=(Nfa<T> &nfa) {
  if (char_ranges_.empty()) {
    char_ranges_.assign(nfa.char_ranges_.cbegin(), nfa.char_ranges_.cend());
  }
  for (auto &pair:nfa.exchange_map_) {
    exchange_map_[pair.first] = std::move(pair.second);
  }
  assertion_states_.merge(nfa.assertion_states_);
  group_states_.merge(nfa.group_states_);
  special_pattern_states_.merge(nfa.special_pattern_states_);
  range_states_.merge(nfa.range_states_);

  return *this;
}

template<class T>
void
Nfa<T>::CharRangesInit(const std::set<std::basic_string<T>> &delim,
                       Encoding encoding) {
  using namespace std;

  set<unsigned int> char_ranges;

  // determine encode range
  unsigned int max_encode;
  switch (encoding) {
    case Encoding::kAscii:
      max_encode = 0x7f;
      break;
    case Encoding::kUtf8:
      max_encode = 0xf7bfbfbf;
      break;
  }

  // initialize special ranges
  AddCharRange(char_ranges, kEmptyEdge);
  AddCharRange(char_ranges, max_encode);

  if (!delim.empty()) {
    for (auto &s:delim) {
      if (s.size() == 1 && s != basic_string<T>(1, kFullStop)) {
        // single character
        // see single character as a range [s[0], s[0] + 1)
        AddCharRange(char_ranges, s[0]);
      }
    }
  }

  char_ranges_.assign(char_ranges.begin(), char_ranges.end());
}

inline void
AddCharRange(std::set<unsigned int> &char_ranges,
             unsigned int begin, unsigned int end) {
  char_ranges.insert(begin);
  char_ranges.insert(end);
}

inline void AddCharRange(std::set<unsigned int> &char_ranges,
                         unsigned int begin) {
  AddCharRange(char_ranges, begin, begin + 1);
}

template<class T>
int Nfa<T>::GetCharLocation(int c) {
  for (int i = 0; i < char_ranges_.size(); ++i) {
    if (char_ranges_[i] > c) {
      return i - 1;
    }
  }
  return -1;
}

template<class T>
Nfa<T>::Nfa(const std::basic_string<T> &regex) {
  // initialize char_ranges_
  auto delim = GetDelim(regex);
  if (typeid(T) == typeid(char)) {
    CharRangesInit(delim, Encoding::kAscii);
  } else {
    CharRangesInit(delim, Encoding::kUtf8);
  }

  auto ast_head = ParseRegex(regex);
  *this = Nfa(ast_head, char_ranges_);
  if (!Empty()) {
    // add a new state as the accept state to prevent that accept state is a
    // functional state
    NewState();
    exchange_map_[accept_state_][kEmptyEdge].insert(i_);
    accept_state_ = i_;
  }
}

template<class T>
AstNodePtr<T> Nfa<T>::ParseRegex(const std::basic_string<T> &regex) {
  using namespace std;

  stack<AstNodePtr<T>> op_stack;
  stack<AstNodePtr<T>> rpn_stack;
  basic_string<T> lex;
  auto cur_it = regex.cbegin(), end = regex.cend();
  bool or_flag = true;  // whether the last lex is |
  AstNodePtr<T> son;

  while (!(lex = NextToken<T>(cur_it, end)).empty()) {
    switch (GetRegexType(lex)) {
      case RegexPart::kAlternative:
        or_flag = true;
        if (!PushOr(op_stack, rpn_stack)) {
          return nullptr;
        }
        break;
      case RegexPart::kQuantifier:
        if (!PushQuantifier(op_stack, rpn_stack, lex)) {
          return nullptr;
        }
        break;
      case RegexPart::kChar:
        // We need to explicitly add & before a character when the last lex
        // isn't |.
        if (!or_flag && !PushAnd(op_stack, rpn_stack)) {
          return nullptr;
        }
        rpn_stack.push(make_unique<AstNode<T>>(RegexPart::kChar, lex));
        or_flag = false;
        break;
      case RegexPart::kGroup:
        if (!or_flag && !PushAnd(op_stack, rpn_stack)) {
          return nullptr;
        }
        // build AST for the sub-pattern in the group
        if (lex[1] == '?') {  // passive group
          // deal with it as a common regex
          son = ParseRegex(basic_string<T>(lex.cbegin() + 3, lex.cend() - 1));
          rpn_stack.push(std::move(son));
        } else {  // group
          rpn_stack.push(make_unique<AstNode<T>>(
                  RegexPart::kGroup,
                  basic_string<T>(lex.cbegin() + 1, lex.cend() - 1)));
        }
        or_flag = false;
        break;
      case RegexPart::kAssertion:
        if (!or_flag && !PushAnd(op_stack, rpn_stack)) {
          return nullptr;
        }
        rpn_stack.push(make_unique<AstNode<T>>(RegexPart::kAssertion, lex));
        or_flag = false;
        break;
      case RegexPart::kAnd:
        // kAnd should not exist here, so it is seen as an error.
      case RegexPart::kError:  // skip the error token
        break;
    }
  }

  // empty op_stack
  if (!PushOr(op_stack, rpn_stack) || rpn_stack.size() != 1) {
    return nullptr;
  }
  return std::move(rpn_stack.top());
}

template<class T>
RegexPart GetRegexType(const std::basic_string<T> &regex) {
  switch (regex[0]) {
    // & is implicit so we don't have to think about it.
    case '|':
      return RegexPart::kAlternative;
    case '*':
    case '+':
    case '?':
    case '{':
      return RegexPart::kQuantifier;
    case '^':
    case '$':
      return RegexPart::kAssertion;
    case '\\':
      if (regex[1] == 'b' || regex[1] == 'B') {
        return RegexPart::kAssertion;
      }
      return RegexPart::kChar;
    case '(':
      if (regex[1] == '?') {
        switch (regex[2]) {
          case '=':
          case '!':
            return RegexPart::kAssertion;
          case ':':
            return RegexPart::kGroup;
          default:  // ? should have characters before it
            return RegexPart::kError;
        }
      }
      return RegexPart::kGroup;
    default:
      return RegexPart::kChar;
  }
}

template<class T>
bool PushAnd(std::stack<AstNodePtr<T>> &op_stack,
             std::stack<AstNodePtr<T>> &rpn_stack) {
  using namespace std;

  while (!op_stack.empty() &&
         op_stack.top()->GetType() != RegexPart::kAlternative) {
    if (op_stack.top()->GetType() == RegexPart::kAnd) {
      if (rpn_stack.size() < 2) {
        return false;
      }
      op_stack.top()->SetRightSon(std::move(rpn_stack.top()));
      rpn_stack.pop();
      op_stack.top()->SetLeftSon(std::move(rpn_stack.top()));
      rpn_stack.pop();
    } else {
      if (rpn_stack.empty()) {
        return false;
      }
      op_stack.top()->SetLeftSon(std::move(rpn_stack.top()));
      rpn_stack.pop();
    }
    rpn_stack.push(std::move(op_stack.top()));
    op_stack.pop();
  }
  op_stack.push(make_unique<AstNode<T>>(RegexPart::kAnd, basic_string<T>()));

  return true;
}

template<class T>
bool PushOr(std::stack<AstNodePtr<T>> &op_stack,
            std::stack<AstNodePtr<T>> &rpn_stack) {
  using namespace std;

  while (!op_stack.empty()) {
    if (op_stack.top()->GetType() == RegexPart::kAnd ||
        op_stack.top()->GetType() == RegexPart::kAlternative) {
      if (rpn_stack.size() < 2) {
        return false;
      }
      op_stack.top()->SetRightSon(std::move(rpn_stack.top()));
      rpn_stack.pop();
      op_stack.top()->SetLeftSon(std::move(rpn_stack.top()));
      rpn_stack.pop();
    } else {
      if (rpn_stack.empty()) {
        return false;
      }
      op_stack.top()->SetLeftSon(std::move(rpn_stack.top()));
      rpn_stack.pop();
    }
    rpn_stack.push(std::move(op_stack.top()));
    op_stack.pop();
  }
  op_stack.push(
          make_unique<AstNode<T>>(RegexPart::kAlternative, basic_string<T>()));

  return true;
}

template<class T>
bool PushQuantifier(std::stack<AstNodePtr<T>> &op_stack,
                    std::stack<AstNodePtr<T>> &rpn_stack,
                    const std::basic_string<T> &regex) {
  using namespace std;

  while (!op_stack.empty() &&
         op_stack.top()->GetType() != RegexPart::kAlternative &&
         op_stack.top()->GetType() != RegexPart::kAnd) {
    if (rpn_stack.empty()) {
      return false;
    }
    op_stack.top()->SetLeftSon(std::move(rpn_stack.top()));
    rpn_stack.pop();
    rpn_stack.push(std::move(op_stack.top()));
    op_stack.pop();
  }
  op_stack.push(make_unique<AstNode<T>>(RegexPart::kQuantifier, regex));

  return true;
}

template<class T>
Nfa<T>::Nfa(AstNodePtr<T> &ast_head,
            const std::vector<unsigned int> &char_ranges) {
  if (ast_head) {
    switch (ast_head->regex_type_) {
      case RegexPart::kChar:
        *this = NfaFactory<T>::MakeCharacterNfa(ast_head->regex_, char_ranges);
        break;
      case RegexPart::kAlternative:
        *this = NfaFactory<T>::MakeAlternativeNfa(
                Nfa(ast_head->left_son_, char_ranges),
                Nfa(ast_head->right_son_, char_ranges));
        break;
      case RegexPart::kAnd:
        *this = NfaFactory<T>::MakeAndNfa(
                Nfa(ast_head->left_son_, char_ranges),
                Nfa(ast_head->right_son_, char_ranges));
        break;
      case RegexPart::kQuantifier:
        *this = NfaFactory<T>::MakeQuantifierNfa(
                ast_head->regex_, ast_head->left_son_, char_ranges);
        break;
      case RegexPart::kGroup:
        char_ranges_.assign(char_ranges.cbegin(), char_ranges.cend());
        NewState();
        begin_state_ = i_;
        accept_state_ = i_;
        group_states_.insert({begin_state_, GroupNfa<T>(ast_head->regex_)});
        break;
      case RegexPart::kAssertion:
        char_ranges_.assign(char_ranges.cbegin(), char_ranges.cend());
        NewState();
        begin_state_ = i_;
        accept_state_ = i_;
        assertion_states_.insert(
                {begin_state_, AssertionNfa<T>(ast_head->regex_)});
        break;
      case RegexPart::kError:
        break;
    }
  }
}

template<class T>
void Nfa<T>::NewState() {
  using namespace std;

  vector<set<int>> edges_vec;
  edges_vec.reserve(char_ranges_.size());
  for (int i = 0; i < char_ranges_.size(); ++i) {
    edges_vec.emplace_back(set<int>());
  }
  exchange_map_.template emplace(++i_, edges_vec);
}

template<class T>
Nfa<T> NfaFactory<T>::MakeCharacterNfa(const std::basic_string<T> &characters,
                                       const std::vector<unsigned int> &char_ranges) {
  using namespace std;

  Nfa<T> nfa;
  nfa.char_ranges_.assign(char_ranges.cbegin(), char_ranges.cend());

  nfa.NewState();
  nfa.begin_state_ = Nfa<T>::i_;

  if (characters.size() == 1) {
    if (characters == basic_string<T>(1, kFullStop)) {
      nfa.accept_state_ = nfa.begin_state_;
      nfa.special_pattern_states_.emplace(
              nfa.begin_state_, SpecialPatternNfa(characters));
    } else {  // single character
      nfa.NewState();
      nfa.accept_state_ = Nfa<T>::i_;
      nfa.exchange_map_[nfa.begin_state_][nfa.GetCharLocation(
              characters[0])].insert(nfa.accept_state_);
    }
  } else if (characters[0] == '[') {  // [...]
    nfa.accept_state_ = nfa.begin_state_;
    nfa.range_states_.emplace(nfa.begin_state_, RangeNfa(characters));
  } else {  // special pattern characters
    nfa.accept_state_ = nfa.begin_state_;
    nfa.special_pattern_states_.emplace(
            nfa.begin_state_, SpecialPatternNfa(characters));
  }

  return nfa;
}

template<class T>
Nfa<T>
NfaFactory<T>::MakeAlternativeNfa(Nfa<T> left_nfa, Nfa<T> right_nfa) {
  Nfa<T> nfa;

  nfa += left_nfa;
  nfa += right_nfa;
  // Add empty edges from new begin state to left_nfa's and right_nfa's begin
  // state.
  nfa.NewState();
  nfa.begin_state_ = Nfa<T>::i_;
  nfa.exchange_map_[nfa.begin_state_][Nfa<T>::kEmptyEdge].insert(
          left_nfa.begin_state_);
  nfa.exchange_map_[nfa.begin_state_][Nfa<T>::kEmptyEdge].insert(
          right_nfa.begin_state_);
  // Add empty edges from left_nfa's and right_nfa's accept states to the new
  // accept state.
  nfa.NewState();
  nfa.exchange_map_[left_nfa.accept_state_][Nfa<T>::kEmptyEdge].insert(
          Nfa<T>::i_);
  nfa.exchange_map_[right_nfa.accept_state_][Nfa<T>::kEmptyEdge].insert(
          Nfa<T>::i_);
  nfa.accept_state_ = Nfa<T>::i_;

  return nfa;
}

template<class T>
Nfa<T> NfaFactory<T>::MakeAndNfa(Nfa<T> left_nfa, Nfa<T> right_nfa) {
  Nfa<T> nfa;

  nfa += left_nfa;
  nfa += right_nfa;
  nfa.begin_state_ = left_nfa.begin_state_;
  nfa.accept_state_ = right_nfa.accept_state_;
  // Add empty edges from left_nfa's accept state to right_nfa's begin state.
  nfa.exchange_map_[left_nfa.accept_state_][Nfa<T>::kEmptyEdge].insert(
          right_nfa.begin_state_);

  return nfa;
}

template<class T>
Nfa<T>
NfaFactory<T>::MakeQuantifierNfa(const std::basic_string<T> &quantifier,
                                 AstNodePtr<T> &left,
                                 const std::vector<unsigned int> &char_ranges) {
  Nfa<T> nfa;
  nfa.char_ranges_.assign(char_ranges.cbegin(), char_ranges.cend());

  auto repeat_range = ParseQuantifier(quantifier);

  nfa.NewState();
  nfa.begin_state_ = Nfa<T>::i_;
  nfa.accept_state_ = Nfa<T>::i_;

  int i = 1;
  for (; i < repeat_range.first; ++i) {
    Nfa<T> left_nfa(left, nfa.char_ranges_);
    // connect left_nfa to the end of the current nfa
    nfa = MakeAndNfa(nfa, left_nfa);
  }

  nfa.NewState();
  int final_accept_state = Nfa<T>::i_;

  if (repeat_range.second == INT_MAX) {
    Nfa<T> left_nfa(left, nfa.char_ranges_);
    // connect left_nfa to the end of the current nfa
    nfa = MakeAndNfa(nfa, left_nfa);
    nfa.exchange_map_[nfa.accept_state_][Nfa<T>::kEmptyEdge].insert(
            left_nfa.begin_state_);
    nfa.exchange_map_[nfa.accept_state_][Nfa<T>::kEmptyEdge].insert(
            final_accept_state);
  } else {
    for (; i <= repeat_range.second; ++i) {
      Nfa<T> left_nfa(left, nfa.char_ranges_);
      // connect left_nfa to the end of the current nfa
      nfa = MakeAndNfa(nfa, left_nfa);
      nfa.exchange_map_[left_nfa.accept_state_][Nfa<T>::kEmptyEdge].insert(
              final_accept_state);
    }
  }

  nfa.accept_state_ = final_accept_state;

  if (repeat_range.first == 0) {
    // add an empty edge from the begin state to the accept state
    nfa.exchange_map_[nfa.begin_state_][Nfa<T>::kEmptyEdge].insert(
            nfa.accept_state_);
  }

  return nfa;
}

template<class T>
std::pair<int, int>
NfaFactory<T>::ParseQuantifier(const std::basic_string<T> &quantifier) {
  using namespace std;

  pair<int, int> repeat_range;

  // initialize 'repeat_range'
  if (quantifier == basic_string<T>(1, kAsterisk)) {
    repeat_range.first = 0;
    repeat_range.second = INT_MAX;
  } else if (quantifier == basic_string<T>(1, kPlusSign)) {
    repeat_range.first = 1;
    repeat_range.second = INT_MAX;
  } else if (quantifier == basic_string<T>(1, kQuestionMark)) {
    repeat_range.first = 0;
    repeat_range.second = 1;
  } else {  // {...}
    // extract first number
    auto beg_it = find_if(quantifier.cbegin(), quantifier.cend(),
                          [](auto c) { return isdigit(c); });
    auto end_it = find_if_not(beg_it, quantifier.cend(),
                              [](auto c) { return isdigit(c); });
    repeat_range.first = stoi(basic_string<T>(beg_it, end_it));

    auto comma = quantifier.find(',');
    if (comma == basic_string<T>::npos) {  // exact times
      repeat_range.second = repeat_range.first;
    } else {  // {min,max} or {min,}
      // extract first number
      beg_it = find_if(end_it, quantifier.cend(),
                       [](auto c) { return isdigit(c); });
      end_it = find_if_not(beg_it, quantifier.cend(),
                           [](auto c) { return isdigit(c); });

      if (beg_it == end_it) {  // {min,}
        repeat_range.second = INT_MAX;
      } else {  // {min,max}
        repeat_range.second = stoi(basic_string<T>(beg_it, end_it));
      }
    }
  }

  return repeat_range;
}

template<class T>
bool AssertionNfa<T>::IsSuccess(StrConstIt<T> str_begin, StrConstIt<T> str_end,
                                StrConstIt<T> begin) {
  switch (type_) {
    case AssertionType::kLineBegin:
      if (begin == str_begin || IsLineTerminator<T>(begin - 1)) {
        return true;
      }
      break;
    case AssertionType::kLineEnd:
      if (begin == str_end || IsLineTerminator<T>(begin + 1)) {
        return true;
      }
      break;
    case AssertionType::kWordBoundary:
      if (begin == str_begin) {
        if (IsWord<T>(begin)) {
          return true;
        }
      } else if (begin == str_end) {
        if (IsWord<T>(begin - 1)) {
          return true;
        }
      } else if (IsWord<T>(begin) != IsWord<T>(begin - 1)) {
        return true;
      }
      break;
    case AssertionType::kNotWordBoundary:
      if (begin == str_begin) {
        if (!IsWord<T>(begin)) {
          return true;
        }
      } else if (begin == str_end) {
        if (!IsWord<T>(begin - 1)) {
          return true;
        }
      } else if (IsWord<T>(begin) == IsWord<T>(begin - 1)) {
        return true;
      }
      break;
    case AssertionType::kPositiveLookahead:
      return nfa_.NextMatch(begin, str_end) != nullptr;
    case AssertionType::kNegativeLookahead:
      return nfa_.NextMatch(begin, str_end) == nullptr;
  }
  return false;
}

template<class T>
bool IsLineTerminator(StrConstIt<T> it) {
  if (*it == '\n' || *it == '\r') {
    return true;
  }
  return false;
}

template<class T>
bool IsWord(StrConstIt<T> it) {
  return *it == '_' || isalnum(*it);
}

template<class T>
std::set<StrConstIt<T>>
GroupNfa<T>::NextMatch(StrConstIt<T> begin, StrConstIt<T> str_end) {
  using namespace std;

  set<StrConstIt<T>> end_its;

  // We don't use begin == str_end since an empty string may be captured.
  if (begin > str_end) {
    return end_its;
  }

  vector<ReachableStatesMap<T>> state_vec = this->StateRoute(begin, str_end);
  auto it = state_vec.cbegin();

  // find the longest match
  while (it != state_vec.cend()) {
    for (const auto &cur_state:*it) {
      if (cur_state.first.first == this->accept_state_) {
        end_its.insert(cur_state.first.second);
      }
    }
    it++;
  }

  return end_its;
}

template<class T>
RangeNfa<T>::RangeNfa(const std::basic_string<T> &regex) {
  using namespace std;

  auto begin = regex.cbegin() + 1, end = regex.cend() - 1;

  if (*begin == '^') {  // [^...]
    except_ = true;
    begin++;
  } else {
    except_ = false;
  }

  while (begin != end) {
    if (*begin == '\\') {  // skip special patterns
      auto tmp_it = SkipEscapeCharacters<T>(begin, end);
      special_patterns_.push_back(
              SpecialPatternNfa(basic_string<T>(begin, tmp_it)));
      begin = tmp_it;
    } else if (*begin == '.') {
      special_patterns_.push_back(
              SpecialPatternNfa<T>(basic_string<T>(begin, begin + 1)));
      begin++;
    } else {
      if (begin + 1 < end && *(begin + 1) == '-') {  // range
        if (begin + 2 < end) {
          ranges_.emplace(*begin, *(begin + 2));
          begin += 3;
        }
      } else {  // single character
        ranges_.emplace(*begin, *(begin + 1));
        begin++;
      }
    }
  }
}

template<class T>
StrConstIt<T>
RangeNfa<T>::NextMatch(const State<T> &state, StrConstIt<T> str_end) {
  auto begin = state.first.second;

  if (begin == str_end) {  // no character to match
    return begin;
  }

  if (except_) {
    for (auto &range:ranges_) {
      if (*begin >= range.first && *begin <= range.second) {
        return begin;
      }
    }

    for (auto special_pattern:special_patterns_) {
      begin = special_pattern.NextMatch(state, str_end);
      if (begin != state.first.second) {
        return state.first.second;
      }
    }

    return begin + 1;
  } else {
    for (auto &range:ranges_) {
      if (*begin >= range.first && *begin <= range.second) {
        return begin + 1;
      }
    }

    for (auto special_pattern:special_patterns_) {
      begin = special_pattern.NextMatch(state, str_end);
      if (begin != state.first.second) {
        return begin;
      }
    }

    return begin;
  }
}
}

#endif // XYREGENGINE_NFA_H