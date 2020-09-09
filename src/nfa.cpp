//
// Created by dxy on 2020/8/6.
//

#include "nfa.h"

#include <climits>
#include <memory>
#include <sstream>
#include <stack>
#include <vector>

#include "lex.h"

using namespace XyRegEngine;
using namespace std;

int Nfa::i_ = 0;

string Nfa::NextMatch(StrConstIt &begin, StrConstIt end) {
    if (begin == end) {
        return "";
    }

    // vector.element -- reachable states after handling '*begin'
    vector<set<int>> state_vec;
    auto tmp = begin;

    // Use the begin state and states that can be accessed through empty
    // edges and function edges as an initial driver.
    auto pair = NextState(begin_state_);
    // deal with assertion function states
    pair.first.merge(CheckAssertion(pair.second, begin, end).first);
    // TODO(dxy): deal with group states in pair.second
    state_vec.push_back(pair.first);

    // find all reachable states from current states
    while (!state_vec[state_vec.size() - 1].empty()) {
        set<int> cur_states;
        for (auto last_state:state_vec[state_vec.size() - 1]) {
            pair = NextState(last_state, begin, end);
            pair.first.merge(CheckAssertion(pair.second, begin, end).first);
            // TODO(dxy): deal with group states in pair.second
            cur_states.merge(pair.first);
        }
        state_vec.push_back(cur_states);
        begin++;
    }
    // remove the last empty vector
    state_vec.pop_back();
    begin--;

    auto reverse_it = state_vec.rbegin();
    // find the last vector that contains the accept state
    while (reverse_it != state_vec.rend()) {
        for (auto cur_state:*reverse_it) {
            if (cur_state == accept_state_) {
                return string(tmp, begin);
            }
        }
        reverse_it++;
        begin--;
    }

    return "";
}

pair<set<int>, set<int>> Nfa::NextState(
        int cur_state, StrConstIt begin, StrConstIt end) {
    vector<int> char_states;  // states reached through *cur_it
    set<int> states;
    // states reached through assertion edges and group edges
    set<int> func_states;

    if (begin != end) {
        // add states that can be reached through '*cur_it' edges
        for (auto state:exchange_map_[cur_state][GetCharLocation(*begin)]) {
            if (IsFuncState(state)) {
                func_states.insert(state);
            } else {
                char_states.push_back(state);
            }
        }

        // add states that can be reached through empty edges
        for (auto char_state:char_states) {
            auto pair = NextState(char_state);
            for (auto state:pair.first) {
                states.insert(state);
            }
            for (auto state:pair.second) {
                func_states.insert(state);
            }
        }
    }

    return pair(std::move(states), std::move(func_states));
}

pair<set<int>, set<int>> Nfa::NextState(int cur_state) {
    vector<int> states;
    // states reached through assertion edges and group edges
    set<int> func_states;

    // add states that can be reached through empty edges and function edges
    int i = -1;
    states.push_back(cur_state);
    while (++i != states.size()) {
        for (auto state:exchange_map_[states[i]][kEmptyEdge]) {
            if (IsFuncState(state)) {
                func_states.insert(state);
            } else if (find(states.cbegin(), states.cend(), state) ==
                       states.cend()) {
                states.push_back(state);
            }
        }
    }

    return pair(set(states.cbegin(), states.cend()), std::move(func_states));
}

pair<set<int>, set<int>> Nfa::CheckAssertion(
        set<int> &func_states, StrConstIt begin, StrConstIt end) {
    set<int> common_states;

    for (auto state:func_states) {
        int type = GetStateType(state);
        pair<set<int>, set<int>> before_check_pair;
        pair<set<int>, set<int>> after_check_pair;
        switch (type) {
            case 0:  // assertion
                before_check_pair = CheckAssertion(state, begin, end, true);
                common_states.merge(before_check_pair.first);
                after_check_pair = CheckAssertion(
                        before_check_pair.second, begin, end);
                common_states.merge(after_check_pair.first);
                func_states.erase(state);
                func_states.merge(after_check_pair.second);
                break;
            case 1:  // not assertion
                before_check_pair = CheckAssertion(state, begin, end, false);
                common_states.merge(before_check_pair.first);
                after_check_pair = CheckAssertion(
                        before_check_pair.second, begin, end);
                common_states.merge(after_check_pair.first);
                func_states.erase(state);
                func_states.merge(after_check_pair.second);
                break;
            default:
                break;
        }
    }

    return {common_states, func_states};
}

pair<set<int>, set<int>> Nfa::CheckAssertion(
        int head_state, StrConstIt begin, StrConstIt end, bool type) {
    // TODO(dxy): deal with ^ $ \b \B
    auto tail_state = GetTailState(head_state);

    // initial driver
    auto pair = NextState(head_state);
    // detect whether end function edge is reached
    if (type) {
        if (!pair.second.empty()) {
            return NextState(
                    *exchange_map_[tail_state][kEmptyEdge].cbegin());
        }
    } else {
        if (!pair.second.empty()) {
            return {set<int>(), set<int>()};
        }
    }

    // find all reachable states from current states
    vector<set<int>> states;
    states.emplace_back(pair.first.cbegin(), pair.first.cend());
    while (!states[states.size() - 1].empty()) {
        set<int> cur_states;
        for (auto last_state:states[states.size() - 1]) {
            pair = NextState(last_state, begin, end);

            // detect whether end function edge is reached
            if (type) {
                if (!pair.second.empty()) {
                    return NextState(
                            *exchange_map_[tail_state][kEmptyEdge].cbegin());
                }
            } else {
                if (!pair.second.empty()) {
                    return {set<int>(), set<int>()};
                }
            }

            // store states can be reached through '*begin'
            for (auto next_state:pair.first) {
                cur_states.insert(next_state);
            }
        }
        states.push_back(cur_states);
        begin++;
    }

    if (type) {
        return {set<int>(), set<int>()};
    } else {
        return NextState(
                *exchange_map_[tail_state][kEmptyEdge].cbegin());
    }
}

int Nfa::GetStateType(int state) {
    auto FindFuncState = [state](std::set<std::pair<int, int>> &func_states) {
        return std::any_of(func_states.begin(), func_states.end(),
                           [state](std::pair<int, int> pair) {
                               if (state == pair.first) {
                                   return true;
                               }
                               return false;
                           });
    };

    if (FindFuncState(assertion_states_)) {
        return 0;
    }
    if (FindFuncState(not_assertion_states_)) {
        return 1;
    }
    if (FindFuncState(group_states_)) {
        return 2;
    }
    return -1;
}

int Nfa::GetTailState(int head_state) {
    auto FindTailState =
            [head_state](std::set<std::pair<int, int>> &func_states) {
                for (auto &pair:func_states) {
                    if (head_state == pair.first) {
                        return pair.second;
                    }
                }
                return 0;
            };

    return FindTailState(assertion_states_) +
           FindTailState(not_assertion_states_) +
           FindTailState(group_states_);
}

bool Nfa::IsFuncState(int state) {
    auto FindFuncState = [state](std::set<std::pair<int, int>> &func_states) {
        return std::any_of(func_states.begin(), func_states.end(),
                           [state](std::pair<int, int> pair) {
                               if (state == pair.first ||
                                   state == pair.second) {
                                   return true;
                               }
                               return false;
                           });
    };

    return FindFuncState(assertion_states_) ||
           FindFuncState(not_assertion_states_) ||
           FindFuncState(group_states_);
}

vector<string> GetDelim(const string &regex);

/**
 * It determines which RegexPart should be chosen according to regex's a few
 * characters from its head.
 *
 * @param regex
 * @return
 */
RegexPart GetRegexType(const string &regex);

vector<string> GetDelim(const string &regex) {
    auto begin = regex.cbegin(), end = regex.cend();
    vector<string> delim;
    string token;

    while (!(token = NextToken(begin, end)).empty()) {
        switch (GetRegexType(token)) {
            case RegexPart::kChar:
                delim.push_back(token);
                break;
            case RegexPart::kAssertion:
                if (token[0] == '(') {
                    // get regex in assertion
                    token.erase(0, 3);
                    token.erase(token.cend() - 1);

                    for (const auto &s:GetDelim(token)) {
                        delim.push_back(s);
                    }
                }
                break;
            case RegexPart::kGroup:
                // get regex in group
                token.erase(0, 1);  // erase '('
                if (token[1] == '?') {
                    token.erase(0, 2);  // erase '?:'
                }
                token.erase(token.cend() - 1);  // erase ')'

                GetDelim(token);
                for (const auto &s:GetDelim(token)) {
                    delim.push_back(s);
                }

                break;
            default:
                break;
        }
    }

    return delim;
}

Nfa &Nfa::operator+=(Nfa &nfa) {
    for (auto &pair:nfa.exchange_map_) {
        exchange_map_[pair.first] = std::move(pair.second);
    }
    assertion_states_.merge(nfa.assertion_states_);
    not_assertion_states_.merge(nfa.not_assertion_states_);
    group_states_.merge(nfa.group_states_);
    return *this;
}

/**
 * Parse a [...] to a serious character ranges.
 *
 * @param range in a format like [...]
 * @return Every pair in the map represents a character range.
 */
map<int, int> SplitRange(const string &range);

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

    // initialize several special ranges
    AddCharRange(char_ranges, kEmptyEdge);
    AddCharRange(char_ranges, kLineBeginEdge);
    AddCharRange(char_ranges, kLineEndEdge);
    AddCharRange(char_ranges, kWordBoundaryEdge);
    AddCharRange(char_ranges, kNotWordBoundaryEdge);
    AddCharRange(char_ranges, max_encode);

    if (delim.empty()) {
        // Initialize 'char_ranges_' by seeing every character as a range by
        // default.
        for (int i = 1; i < max_encode; ++i) {
            char_ranges.insert(i);
        }
    } else {
        for (auto &s:delim) {
            if (s.size() == 1) {  // single character
                // see single character as a range
                AddCharRange(char_ranges, s[0]);
            } else if (s[0] == '\\') {  // escape character
                // TODO(dxy):
            } else {  // character range
                for (auto &pair:SplitRange(s)) {
                    AddCharRange(char_ranges, pair.first, pair.second + 1);
                }
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

map<int, int> SplitRange(const string &range) {
    map<int, int> ranges;

    for (int i = 1; i < range.size() - 1; ++i) {
        if (range[i] == '\\') {  // escape character
            // TODO(dxy):
        } else {
            if (i + 1 < range.size() - 1 && range[i + 1] == '-') {  // range
                if (i + 2 < range.size() - 1) {
                    ranges.insert({range[i], range[i + 2]});
                    i += 2;
                }
            } else {  // single character
                ranges.insert({range[i], range[i]});
            }
        }
    }

    return ranges;
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
    *this = Nfa(ast_head, char_ranges_);
}

bool PushAnd(stack<AstNodePtr> &op_stack, stack<AstNodePtr> &rpn_stack);

bool PushOr(stack<AstNodePtr> &op_stack, stack<AstNodePtr> &rpn_stack);

bool PushQuantifier(stack<AstNodePtr> &op_stack, stack<AstNodePtr> &rpn_stack,
                    const string &regex);

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

                // build AST for the nested regex
                if (lex[1] == '?') {  // passive group
                    son = ParseRegex(string(lex.cbegin() + 3, lex.cend() - 1));
                } else {  // group
                    son = ParseRegex(string(lex.cbegin() + 1, lex.cend() - 1));
                }

                if (son) {
                    AstNodePtr group_node =
                            make_unique<AstNode>(RegexPart::kGroup, "");
                    group_node->SetLeftSon(std::move(son));
                    rpn_stack.push(std::move(group_node));
                }
                or_flag = false;
                break;
            case RegexPart::kAssertion:
                if (!or_flag) {
                    if (!PushAnd(op_stack, rpn_stack)) {
                        return nullptr;
                    }
                }

                if (lex[0] != '(') {
                    rpn_stack.push(make_unique<AstNode>(
                            RegexPart::kAssertion, lex));
                } else {
                    // build AST for the nested regex
                    son = ParseRegex(string(lex.cbegin() + 3, lex.cend() - 1));

                    if (son) {
                        AstNodePtr assertion_node =
                                make_unique<AstNode>(RegexPart::kAssertion,
                                                     lex);
                        assertion_node->SetLeftSon(std::move(son));
                        rpn_stack.push(std::move(assertion_node));
                    }
                }
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
    char_ranges_.assign(char_ranges.cbegin(), char_ranges.cend());

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
                *this = NfaFactory::MakeGroupNfa(
                        ast_head->regex_,
                        Nfa(ast_head->left_son_, char_ranges));
                break;
            case RegexPart::kAssertion:
                *this = NfaFactory::MakeAssertionNfa(
                        ast_head->regex_,
                        Nfa(ast_head->left_son_, char_ranges));
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
        std::string characters, vector<unsigned int> char_ranges) {
    Nfa nfa{std::move(char_ranges)};

    nfa.NewState();
    nfa.begin_state_ = Nfa::i_;
    nfa.NewState();
    nfa.accept_state_ = Nfa::i_;

    if (characters.size() == 1) {  // single character
        nfa.exchange_map_[nfa.begin_state_][nfa.GetCharLocation(
                characters[0])].insert(Nfa::i_);
    } else if (characters[0] == '[') {  // [...]
        auto ranges = SplitRange(characters);
        for (auto &pair:ranges) {
            auto begin = nfa.GetCharLocation(pair.first);
            auto end = nfa.GetCharLocation(pair.second);

            while (begin <= end) {
                nfa.exchange_map_[nfa.begin_state_][begin].insert(Nfa::i_);
                begin++;
            }
        }
    } else {  // escape character
        // TODO(dxy):
    }

    return nfa;
}

Nfa NfaFactory::MakeAlternativeNfa(Nfa left_nfa, Nfa right_nfa) {
    Nfa nfa{std::move(left_nfa.char_ranges_)};

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
    Nfa nfa{std::move(left_nfa.char_ranges_)};

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

Nfa NfaFactory::MakeGroupNfa(string group, Nfa left_nfa) {
    Nfa nfa{std::move(left_nfa.char_ranges_)};

    nfa += left_nfa;

    if (group[1] == '?') {  // passive group
        nfa.begin_state_ = left_nfa.begin_state_;
        nfa.accept_state_ = left_nfa.accept_state_;
    } else {  // group
        nfa.NewState();
        nfa.begin_state_ = Nfa::i_;
        nfa.NewState();
        nfa.accept_state_ = Nfa::i_;

        nfa.exchange_map_[nfa.begin_state_][Nfa::kEmptyEdge].insert(
                left_nfa.begin_state_);
        nfa.exchange_map_[left_nfa.accept_state_][Nfa::kEmptyEdge].insert(
                nfa.accept_state_);
        nfa.group_states_.insert(
                {left_nfa.begin_state_, left_nfa.accept_state_});
    }

    return nfa;
}

Nfa NfaFactory::MakeAssertionNfa(std::string assertion, Nfa left_nfa) {
    Nfa nfa{std::move(left_nfa.char_ranges_)};

    if (assertion[0] != '(') {
        // add ^ edge
        nfa.NewState();
        nfa.NewState();
        if (assertion == "^") {
            nfa.exchange_map_[Nfa::i_ - 1][Nfa::kLineBeginEdge].insert(
                    Nfa::i_);
        } else if (assertion == "$") {
            nfa.exchange_map_[Nfa::i_ - 1][Nfa::kLineEndEdge].insert(
                    Nfa::i_);
        } else if (assertion == "\\b") {
            nfa.exchange_map_[Nfa::i_ - 1][Nfa::kWordBoundaryEdge].insert(
                    Nfa::i_);
        } else if (assertion == "\\B") {
            nfa.exchange_map_[Nfa::i_ - 1][Nfa::kNotWordBoundaryEdge].insert(
                    Nfa::i_);
        }

        nfa.NewState();
        nfa.begin_state_ = Nfa::i_;
        nfa.NewState();
        nfa.accept_state_ = Nfa::i_;
        nfa.exchange_map_[nfa.begin_state_][Nfa::kEmptyEdge].insert(
                Nfa::i_ - 3);
        nfa.exchange_map_[Nfa::i_ - 2][Nfa::kEmptyEdge].insert(Nfa::i_);
        nfa.assertion_states_.insert({Nfa::i_ - 3, Nfa::i_});
    } else {  // lookahead
        nfa += left_nfa;

        nfa.NewState();
        nfa.begin_state_ = Nfa::i_;
        nfa.NewState();
        nfa.accept_state_ = Nfa::i_;

        nfa.exchange_map_[nfa.begin_state_][Nfa::kEmptyEdge].insert(
                left_nfa.begin_state_);
        nfa.exchange_map_[left_nfa.accept_state_][Nfa::kEmptyEdge].insert(
                nfa.accept_state_);

        if (assertion[2] == '=') {  // positive lookahead
            nfa.assertion_states_.insert(
                    {left_nfa.begin_state_, left_nfa.accept_state_});
        } else {  // negative lookahead
            nfa.not_assertion_states_.insert(
                    {left_nfa.begin_state_, left_nfa.accept_state_});
        }
    }

    return nfa;
}

Nfa NfaFactory::MakeQuantifierNfa(const string &quantifier, AstNodePtr &left,
                                  vector<unsigned int> char_ranges) {
    Nfa nfa{std::move(char_ranges)};
    auto repeat_range = ParseQuantifier(quantifier);

    nfa.NewState();
    nfa.begin_state_ = Nfa::i_;
    nfa.accept_state_ = Nfa::i_;

    for (int i = 1; i < repeat_range.first; ++i) {
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
        for (int i = repeat_range.first; i <= repeat_range.second; ++i) {
            Nfa left_nfa{left, nfa.char_ranges_};
            // connect 'left_nfa' to the end of the current nfa
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

pair<int, int> NfaFactory::ParseQuantifier(const std::string &quantifier) {
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
