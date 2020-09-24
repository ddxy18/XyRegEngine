//
// Created by dxy on 2020/8/6.
//

#include "nfa.h"

#include <cctype>
#include <climits>
#include <stack>

#include "lex.h"

using namespace XyRegEngine;
using namespace std;

int Nfa::i_ = 0;

/**
 * It determines which RegexPart should be chosen according to regex's a few
 * characters from its head.
 *
 * @param regex
 * @return
 */
RegexPart GetRegexType(const string &regex);

vector<string> GetDelim(const string &regex);

/**
 * Add a character range [begin, end) to char_ranges.
 *
 * @param begin
 * @param end
 */
void AddCharRange(
        set<unsigned int> &char_ranges, unsigned int begin, unsigned int end);

/**
 * Add a char range [begin, begin + 1) to char_ranges.
 *
 * @param begin
 */
void AddCharRange(set<unsigned int> &char_ranges, unsigned int begin);

bool PushAnd(stack<AstNodePtr> &op_stack, stack<AstNodePtr> &rpn_stack);

bool PushOr(stack<AstNodePtr> &op_stack, stack<AstNodePtr> &rpn_stack);

bool PushQuantifier(stack<AstNodePtr> &op_stack, stack<AstNodePtr> &rpn_stack,
                    const string &regex);

bool IsLineTerminator(StrConstIt it);

bool IsWord(StrConstIt it);

StatePtr Nfa::NextMatch(StrConstIt begin, StrConstIt end) {
    vector<ReachableStatesMap> state_vec = StateRoute(begin, end);
    auto it = state_vec.cbegin();
    State state = *state_vec[0].find({begin_state_, begin});

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
        return make_unique<State>(state);
    }
}

vector<ReachableStatesMap> Nfa::StateRoute(StrConstIt begin, StrConstIt end) {
    vector<ReachableStatesMap> state_vec;
    ReachableStatesMap cur_states;

    // Use states that can be accessed through empty edges from begin_state_
    // and begin_state_ itself as an initial driver.
    State begin_state = {{begin_state_, begin}, vector<SubMatch>()};
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

ReachableStatesMap Nfa::NextState(const State &cur_state,
                                  StrConstIt str_begin, StrConstIt str_end) {
    ReachableStatesMap next_states;
    auto begin = cur_state.first.second;

    switch (GetStateType(cur_state.first.first)) {
        case StateType::kAssertion:
            if (!assertion_states_.find(
                    cur_state.first.first)->second.IsSuccess(
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
                next_states.insert(
                        {{cur_state.first.first, end_it}, sub_matches});
            }
            break;
        case StateType::kSpecialPattern:
            begin = special_pattern_states_.find(
                    cur_state.first.first)->second.NextMatch(
                    cur_state, str_end);
            if (begin != cur_state.first.second) {
                next_states.insert(
                        {{cur_state.first.first, begin}, cur_state.second});
            }
            break;
        case StateType::kRange:
            begin = range_states_.find(
                    cur_state.first.first)->second.NextMatch(
                    cur_state, str_end);
            if (begin != cur_state.first.second) {
                next_states.insert(
                        {{cur_state.first.first, begin}, cur_state.second});
            }
            break;
        case StateType::kCommon:
            // add states that can be reached through *cur_it
            if (begin < str_end) {
                for (auto state:exchange_map_[cur_state.first.first][
                        GetCharLocation(*begin)]) {
                    next_states.insert(
                            {{state, begin + 1}, cur_state.second});
                }
            }
            break;
    }

    ReachableStatesMap next_states_from_empty;
    for (const auto &state:next_states) {
        next_states_from_empty.merge(NextState(state));
    }
    next_states.erase({cur_state.first.first, begin});
    next_states.merge(next_states_from_empty);

    return next_states;
}

ReachableStatesMap Nfa::NextState(const State &cur_state) {
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

    ReachableStatesMap next_states_map;
    for (auto state:common_states) {
        next_states_map.insert(
                {{state, cur_state.first.second}, cur_state.second});
    }
    for (auto state:func_states) {
        next_states_map.insert(
                {{state, cur_state.first.second}, cur_state.second});
    }

    return next_states_map;
}

Nfa::StateType Nfa::GetStateType(int state) {
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

vector<string> GetDelim(const string &regex) {
    auto begin = regex.cbegin(), end = regex.cend();
    vector<string> delim;
    string token;

    while (!(token = NextToken(begin, end)).empty()) {
        if (GetRegexType(token) == RegexPart::kChar) {
            delim.push_back(token);
        }
    }

    return delim;
}

Nfa &Nfa::operator+=(Nfa &nfa) {
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

void Nfa::CharRangesInit(const vector<string> &delim, Encoding encoding) {
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

    if (delim.empty()) {
        // Initialize char_ranges_ by seeing every character as a range by
        // default.
        for (int i = 1; i < max_encode; ++i) {
            AddCharRange(char_ranges, i);
        }
    } else {
        for (auto &s:delim) {
            if (s.size() == 1 && s != ".") {  // single character
                // see single character as a range [s[0], s[0] + 1)
                AddCharRange(char_ranges, s[0]);
            }
        }
    }

    char_ranges_.assign(char_ranges.begin(), char_ranges.end());
}

void AddCharRange(
        set<unsigned int> &char_ranges, unsigned int begin, unsigned int end) {
    char_ranges.insert(begin);
    char_ranges.insert(end);
}

void AddCharRange(set<unsigned int> &char_ranges, unsigned int begin) {
    AddCharRange(char_ranges, begin, begin + 1);
}

int Nfa::GetCharLocation(int c) {
    for (int i = 0; i < char_ranges_.size(); ++i) {
        if (char_ranges_[i] > c) {
            return i - 1;
        }
    }
    return -1;
}

Nfa::Nfa(const string &regex) {
    // initialize 'char_ranges_'
    auto delim = GetDelim(regex);
    // TODO(dxy): determine encoding according characters in 'regex'
    CharRangesInit(delim, Encoding::kAscii);

    auto ast_head = ParseRegex(regex);
    *this = Nfa{ast_head, char_ranges_};

    // add a new state as the accept state to prevent that accept state is a
    // functional state
    NewState();
    exchange_map_[accept_state_][kEmptyEdge].insert(i_);
    accept_state_ = i_;
}

AstNodePtr Nfa::ParseRegex(const string &regex) {
    stack<AstNodePtr> op_stack;
    stack<AstNodePtr> rpn_stack;
    string lex;
    auto cur_it = regex.cbegin(), end = regex.cend();
    bool or_flag = true;  // whether the last lex is '|'
    AstNodePtr son;

    while (!(lex = NextToken(cur_it, end)).empty()) {
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
                // We need to explicitly add '&' before a character when the
                // last lex isn't '|'.
                if (!or_flag) {
                    if (!PushAnd(op_stack, rpn_stack)) {
                        return nullptr;
                    }
                }

                rpn_stack.push(make_unique<AstNode>(RegexPart::kChar, lex));
                or_flag = false;
                break;
            case RegexPart::kGroup:
                if (!or_flag) {
                    if (!PushAnd(op_stack, rpn_stack)) {
                        return nullptr;
                    }
                }

                // build AST for the sub-pattern in the group
                if (lex[1] == '?') {  // passive group
                    // deal with it as a common regex
                    son = ParseRegex(string{lex.cbegin() + 3, lex.cend() - 1});
                    rpn_stack.push(std::move(son));
                } else {  // group
                    rpn_stack.push(make_unique<AstNode>(
                            RegexPart::kGroup,
                            string{lex.cbegin() + 1, lex.cend() - 1}));
                }
                or_flag = false;
                break;
            case RegexPart::kAssertion:
                if (!or_flag) {
                    if (!PushAnd(op_stack, rpn_stack)) {
                        return nullptr;
                    }
                }
                rpn_stack.push(
                        make_unique<AstNode>(RegexPart::kAssertion, lex));
                or_flag = false;
                break;
            case RegexPart::kAnd:
                // kAnd should not exist here, so it is seen as an error.
            case RegexPart::kError:  // skip the error token
                break;
        }
    }

    // empty 'op_stack'
    if (!PushOr(op_stack, rpn_stack) || rpn_stack.size() != 1) {
        return nullptr;
    }
    return std::move(rpn_stack.top());
}

RegexPart GetRegexType(const string &regex) {
    switch (regex[0]) {
        // '&' is implicit so we don't have to think about it.
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
                    default:  // '?' should have characters before it
                        return RegexPart::kError;
                }
            }
            return RegexPart::kGroup;
        default:
            return RegexPart::kChar;
    }
}

bool PushAnd(stack<AstNodePtr> &op_stack, stack<AstNodePtr> &rpn_stack) {
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
    op_stack.push(make_unique<AstNode>(RegexPart::kAnd, ""));

    return true;
}

bool PushOr(stack<AstNodePtr> &op_stack, stack<AstNodePtr> &rpn_stack) {
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
    op_stack.push(make_unique<AstNode>(RegexPart::kAlternative, ""));

    return true;
}

bool PushQuantifier(stack<AstNodePtr> &op_stack, stack<AstNodePtr> &rpn_stack,
                    const string &regex) {
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
    op_stack.push(make_unique<AstNode>(RegexPart::kQuantifier, regex));

    return true;
}

Nfa::Nfa(AstNodePtr &ast_head, const vector<unsigned int> &char_ranges) {
    if (ast_head) {
        switch (ast_head->regex_type_) {
            case RegexPart::kChar:
                *this = NfaFactory::MakeCharacterNfa(
                        ast_head->regex_, char_ranges);
                break;
            case RegexPart::kAlternative:
                *this = NfaFactory::MakeAlternativeNfa(
                        Nfa(ast_head->left_son_, char_ranges),
                        Nfa(ast_head->right_son_, char_ranges));
                break;
            case RegexPart::kAnd:
                *this = NfaFactory::MakeAndNfa(
                        Nfa(ast_head->left_son_, char_ranges),
                        Nfa(ast_head->right_son_, char_ranges));
                break;
            case RegexPart::kQuantifier:
                *this = NfaFactory::MakeQuantifierNfa(
                        ast_head->regex_, ast_head->left_son_, char_ranges);
                break;
            case RegexPart::kGroup:
                char_ranges_.assign(char_ranges.cbegin(), char_ranges.cend());
                NewState();
                begin_state_ = i_;
                accept_state_ = i_;
                group_states_.insert(
                        {begin_state_, GroupNfa{ast_head->regex_}});
                break;
            case RegexPart::kAssertion:
                char_ranges_.assign(char_ranges.cbegin(), char_ranges.cend());
                NewState();
                begin_state_ = i_;
                accept_state_ = i_;
                assertion_states_.insert(
                        {begin_state_, AssertionNfa{ast_head->regex_}});
                break;
            case RegexPart::kError:
                break;
        }
    }
}

void Nfa::NewState() {
    vector<set<int>> edges_vec;
    edges_vec.reserve(char_ranges_.size());
    for (int i = 0; i < char_ranges_.size(); ++i) {
        edges_vec.emplace_back(set<int>());
    }
    exchange_map_.insert({++i_, edges_vec});
}

Nfa NfaFactory::MakeCharacterNfa(
        const string &characters, const vector<unsigned int> &char_ranges) {
    Nfa nfa;
    nfa.char_ranges_.assign(char_ranges.cbegin(), char_ranges.cend());

    nfa.NewState();
    nfa.begin_state_ = Nfa::i_;

    if (characters.size() == 1) {
        if (characters == ".") {
            nfa.accept_state_ = nfa.begin_state_;
            nfa.special_pattern_states_.insert(
                    {nfa.begin_state_, SpecialPatternNfa{characters}});
        } else {  // single character
            nfa.NewState();
            nfa.accept_state_ = Nfa::i_;
            nfa.exchange_map_[nfa.begin_state_][nfa.GetCharLocation(
                    characters[0])].insert(nfa.accept_state_);
        }
    } else if (characters[0] == '[') {  // [...]
        nfa.accept_state_ = nfa.begin_state_;
        nfa.range_states_.insert({nfa.begin_state_, RangeNfa{characters}});
    } else {  // special pattern characters
        nfa.accept_state_ = nfa.begin_state_;
        nfa.special_pattern_states_.insert(
                {nfa.begin_state_, SpecialPatternNfa{characters}});
    }

    return nfa;
}

Nfa NfaFactory::MakeAlternativeNfa(Nfa left_nfa, Nfa right_nfa) {
    Nfa nfa;

    nfa += left_nfa;
    nfa += right_nfa;
    //  Add empty edges from new begin state to 'left_nfa''s and
    //  'right_nfa''s begin state.
    nfa.NewState();
    nfa.begin_state_ = Nfa::i_;
    nfa.exchange_map_[nfa.begin_state_][Nfa::kEmptyEdge].insert(
            left_nfa.begin_state_);
    nfa.exchange_map_[nfa.begin_state_][Nfa::kEmptyEdge].insert(
            right_nfa.begin_state_);
    // Add empty edges from 'left_nfa''s and 'right_nfa''s accept states to
    // the new accept state.
    nfa.NewState();
    nfa.exchange_map_[left_nfa.accept_state_][Nfa::kEmptyEdge].insert(Nfa::i_);
    nfa.exchange_map_[right_nfa.accept_state_][Nfa::kEmptyEdge].insert(Nfa::i_);
    nfa.accept_state_ = Nfa::i_;

    return nfa;
}

Nfa NfaFactory::MakeAndNfa(Nfa left_nfa, Nfa right_nfa) {
    Nfa nfa;

    nfa += left_nfa;
    nfa += right_nfa;
    nfa.begin_state_ = left_nfa.begin_state_;
    nfa.accept_state_ = right_nfa.accept_state_;
    // Add empty edges from 'left_nfa''s accept state to 'right_nfa''s begin
    // state.
    nfa.exchange_map_[left_nfa.accept_state_][Nfa::kEmptyEdge].insert(
            right_nfa.begin_state_);

    return nfa;
}

Nfa NfaFactory::MakeQuantifierNfa(const string &quantifier, AstNodePtr &left,
                                  const vector<unsigned int> &char_ranges) {
    Nfa nfa;
    nfa.char_ranges_.assign(char_ranges.cbegin(), char_ranges.cend());

    auto repeat_range = ParseQuantifier(quantifier);

    nfa.NewState();
    nfa.begin_state_ = Nfa::i_;
    nfa.accept_state_ = Nfa::i_;

    int i = 1;
    for (; i < repeat_range.first; ++i) {
        Nfa left_nfa{left, nfa.char_ranges_};
        // connect 'left_nfa' to the end of the current nfa
        nfa = MakeAndNfa(nfa, left_nfa);
    }

    nfa.NewState();
    int final_accept_state = Nfa::i_;

    if (repeat_range.second == INT_MAX) {
        Nfa left_nfa{left, nfa.char_ranges_};
        // connect 'left_nfa' to the end of the current nfa
        nfa = MakeAndNfa(nfa, left_nfa);
        nfa.exchange_map_[nfa.accept_state_][Nfa::kEmptyEdge].insert(
                left_nfa.begin_state_);
        nfa.exchange_map_[nfa.accept_state_][Nfa::kEmptyEdge].insert(
                final_accept_state);
    } else {
        for (; i <= repeat_range.second; ++i) {
            Nfa left_nfa{left, nfa.char_ranges_};
            // connect left_nfa to the end of the current nfa
            nfa = MakeAndNfa(nfa, left_nfa);
            nfa.exchange_map_[left_nfa.accept_state_][Nfa::kEmptyEdge].insert(
                    final_accept_state);
        }
    }

    nfa.accept_state_ = final_accept_state;

    if (repeat_range.first == 0) {
        // add an empty edge from the begin state to the accept state
        nfa.exchange_map_[nfa.begin_state_][Nfa::kEmptyEdge].insert(
                nfa.accept_state_);
    }

    return nfa;
}

pair<int, int> NfaFactory::ParseQuantifier(const string &quantifier) {
    pair<int, int> repeat_range;

    // initialize 'repeat_range'
    if (quantifier == "*") {
        repeat_range.first = 0;
        repeat_range.second = INT_MAX;
    } else if (quantifier == "+") {
        repeat_range.first = 1;
        repeat_range.second = INT_MAX;
    } else if (quantifier == "?") {
        repeat_range.first = 0;
        repeat_range.second = 1;
    } else {  // {...}
        // extract first number
        auto beg_it = find_if(quantifier.cbegin(), quantifier.cend(),
                              [](auto c) { return isdigit(c); });
        auto end_it = find_if_not(beg_it, quantifier.cend(),
                                  [](auto c) { return isdigit(c); });
        repeat_range.first = stoi(string(beg_it, end_it));

        auto comma = quantifier.find(',');
        if (comma == string::npos) {  // exact times
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
                repeat_range.second = stoi(string(beg_it, end_it));
            }
        }
    }

    return repeat_range;
}

AssertionNfa::AssertionNfa(const string &assertion) {
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
        nfa_ = Nfa(regex);
    }
}

bool AssertionNfa::IsSuccess(
        StrConstIt str_begin, StrConstIt str_end, StrConstIt begin) {
    switch (type_) {
        case AssertionType::kLineBegin:
            if (begin == str_begin || IsLineTerminator(begin - 1)) {
                return true;
            }
            break;
        case AssertionType::kLineEnd:
            if (begin == str_end || IsLineTerminator(begin + 1)) {
                return true;
            }
            break;
        case AssertionType::kWordBoundary:
            if (begin == str_begin) {
                if (IsWord(begin)) {
                    return true;
                }
            } else if (begin == str_end) {
                if (IsWord(begin - 1)) {
                    return true;
                }
            } else if (IsWord(begin) != IsWord(begin - 1)) {
                return true;
            }
            break;
        case AssertionType::kNotWordBoundary:
            if (begin == str_begin) {
                if (!IsWord(begin)) {
                    return true;
                }
            } else if (begin == str_end) {
                if (!IsWord(begin - 1)) {
                    return true;
                }
            } else if (IsWord(begin) == IsWord(begin - 1)) {
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

bool IsLineTerminator(StrConstIt it) {
    if (*it == '\n' || *it == '\r') {
        return true;
    }
    return false;
}

bool IsWord(StrConstIt it) {
    return *it == '_' || isalnum(*it);
}

set<StrConstIt> GroupNfa::NextMatch(StrConstIt begin, StrConstIt str_end) {
    set<StrConstIt> end_its;

    // We don't use begin == str_end since an empty string may be captured.
    if (begin > str_end) {
        return end_its;
    }

    vector<ReachableStatesMap> state_vec = StateRoute(begin, str_end);
    auto it = state_vec.cbegin();

    // find the longest match
    while (it != state_vec.cend()) {
        for (const auto &cur_state:*it) {
            if (cur_state.first.first == accept_state_) {
                end_its.insert(cur_state.first.second);
            }
        }
        it++;
    }

    return end_its;
}

StrConstIt SpecialPatternNfa::NextMatch(
        const State &state, StrConstIt str_end) {
    auto begin = state.first.second;

    if (begin < str_end) {
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
                if (*(begin + i) != *(state.second[back_reference - 1].first +
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

RangeNfa::RangeNfa(const string &regex) {
    auto begin = regex.cbegin() + 1, end = regex.cend() - 1;

    if (*begin == '^') {  // [^...]
        except_ = true;
        begin++;
    } else {
        except_ = false;
    }

    while (begin != end) {
        if (*begin == '\\') {  // skip special patterns
            auto tmp_it = SkipEscapeCharacters(begin, end);
            special_patterns_.push_back(
                    SpecialPatternNfa{string{begin, tmp_it}});
            begin = tmp_it;
        } else if (*begin == '.') {
            special_patterns_.push_back(
                    SpecialPatternNfa{string{begin, begin + 1}});
            begin++;
        } else {
            if (begin + 1 < end && *(begin + 1) == '-') {  // range
                if (begin + 2 < end) {
                    ranges_.insert({*begin, *(begin + 2)});
                    begin += 3;
                }
            } else {  // single character
                ranges_.insert({*begin, *(begin + 1)});
                begin++;
            }
        }
    }
}

StrConstIt RangeNfa::NextMatch(const State &state, StrConstIt str_end) {
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