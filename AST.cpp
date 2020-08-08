//
// Created by dxy on 2020/8/6.
//

#include "include/AST.h"

#include <climits>
#include <stack>

#include "include/lex.h"

using namespace std;
using namespace AST;

/**
 * lex classification
 *
 * AstNode(including '|' and '&')
 * CharNode(including [...] and single character)
 * RepetitionNode('*' '?' '+' '{...}' '*?' '+?')
 * kNestedNode('(...)')
 */
enum class LexType {
    kAstNode, kCharNode, kRepetitionNode, kNestedNode
};

/**
 * It determines which class derives from 'AstNode' should be chosen
 * according to 'lex''s a few characters from its head.
 *
 * @param lex
 * @return LexType
 */
LexType GetType(const string &lex);

[[nodiscard]] bool PushAnd(stack<shared_ptr<AstNode>> &op_stack,
                           stack<shared_ptr<AstNode>> &rpn_stack);

[[nodiscard]] bool PushOr(stack<shared_ptr<AstNode>> &op_stack,
                          stack<shared_ptr<AstNode>> &rpn_stack);

[[nodiscard]] bool PushRepetition(stack<shared_ptr<AstNode>> &op_stack,
                                  stack<shared_ptr<AstNode>> &rpn_stack,
                                  const string &lex);

vector<int> FindNumber(const string &s);

[[nodiscard]] shared_ptr<AstNode> AST::CreateAst(const string &regex) {
    string lex;
    auto begin = regex.cbegin(), end = regex.cend();
    string s;

    // We use a stack to store temporary ops when translating a regex to RPN.
    stack<shared_ptr<AstNode>> op_stack;
    stack<shared_ptr<AstNode>> rpn_stack;

    // record whether the last lex is '|'
    bool or_flag = true;
    shared_ptr<AstNode> son_ast;
    while (!(lex = NextToken(regex, begin, end)).empty()) {
        switch (GetType(lex)) {
            case LexType::kAstNode:
                /**
                 * Since '&' is implicit in regex, so now 'kAstNode' only
                 * represents '|'.
                 */
                or_flag = true;
                if (!PushOr(op_stack, rpn_stack)) {
                    return nullptr;
                }
                break;
            case LexType::kCharNode:
                /**
                 * We need to explicitly add '&' before a character when the
                 * last lex isn't '|'.
                 */
                if (!or_flag) {
                    if (!PushAnd(op_stack, rpn_stack)) {
                        return nullptr;
                    }
                    rpn_stack.push(make_shared<CharNode>(lex));
                } else {
                    rpn_stack.push(make_shared<CharNode>(lex));
                }
                or_flag = false;
                break;
            case LexType::kRepetitionNode:
                if (!PushRepetition(op_stack, rpn_stack, lex)) {
                    return nullptr;
                }
                break;
            case LexType::kNestedNode:
                if (!or_flag && !PushAnd(op_stack, rpn_stack)) {
                    return nullptr;
                }
                or_flag = false;
                rpn_stack.push(NestedNode::MakeNestedNode(lex));
                // build AST for the nested regex
                rpn_stack.top()->SetLeftSon(
                        CreateAst(
                                dynamic_pointer_cast<NestedNode>(
                                        rpn_stack.top()
                                )->GetRegex()
                        )
                );
                break;
        }
    }
    // empty 'op_stack'
    if (!PushOr(op_stack, rpn_stack) || rpn_stack.size() != 1) {
        return nullptr;
    }
    return rpn_stack.top();
}

LexType GetType(const string &lex) {
    switch (lex[0]) {
        case '|':
            return LexType::kAstNode;
        case '*':
        case '+':
        case '?':
        case '{':
            return LexType::kRepetitionNode;
        case '(':
            return LexType::kNestedNode;
        default:
            return LexType::kCharNode;
    }
}

[[nodiscard]] bool PushAnd(stack<shared_ptr<AstNode>> &op_stack,
                           stack<shared_ptr<AstNode>> &rpn_stack) {
    while (!op_stack.empty() && op_stack.top()->GetLex() != "|") {
        if (typeid(*op_stack.top().get()) == typeid(RepetitionNode)) {
            if (rpn_stack.empty()) {
                return false;
            }
            op_stack.top()->SetLeftSon(rpn_stack.top());
            rpn_stack.pop();
            rpn_stack.push(op_stack.top());
            op_stack.pop();
        } else {
            if (rpn_stack.size() < 2) {
                return false;
            }
            op_stack.top()->SetRightSon(rpn_stack.top());
            rpn_stack.pop();
            op_stack.top()->SetLeftSon(rpn_stack.top());
            rpn_stack.pop();
            rpn_stack.push(op_stack.top());
            op_stack.pop();
        }
    }
    op_stack.push(make_shared<AstNode>("&"));
    return true;
}

[[nodiscard]] bool PushOr(stack<shared_ptr<AstNode>> &op_stack,
                          stack<shared_ptr<AstNode>> &rpn_stack) {
    while (!op_stack.empty()) {
        if (typeid(*op_stack.top().get()) == typeid(RepetitionNode)) {
            if (rpn_stack.empty()) {
                return false;
            }
            op_stack.top()->SetLeftSon(rpn_stack.top());
            rpn_stack.pop();
            rpn_stack.push(op_stack.top());
            op_stack.pop();
        } else {
            if (rpn_stack.size() < 2) {
                return false;
            }
            op_stack.top()->SetRightSon(rpn_stack.top());
            rpn_stack.pop();
            op_stack.top()->SetLeftSon(rpn_stack.top());
            rpn_stack.pop();
            rpn_stack.push(op_stack.top());
            op_stack.pop();
        }
    }
    op_stack.push(make_shared<AstNode>("|"));
    return true;
}

[[nodiscard]] bool PushRepetition(stack<shared_ptr<AstNode>> &op_stack,
                                  stack<shared_ptr<AstNode>> &rpn_stack,
                                  const string &lex) {
    while (!op_stack.empty() &&
           typeid(*op_stack.top().get()) == typeid(RepetitionNode)) {
        if (rpn_stack.empty()) {
            return false;
        }
        op_stack.top()->SetLeftSon(rpn_stack.top());
        rpn_stack.pop();
        rpn_stack.push(op_stack.top());
        op_stack.pop();
    }
    op_stack.push(make_shared<RepetitionNode>(lex));
    return true;
}

RepetitionNode::RepetitionNode(const string &lex) : AstNode(lex) {
    vector<int> numbers;
    switch (lex[0]) {
        case '*':
            min = 0;
            max = INT_MAX;
            break;
        case '+':
            min = 1;
            max = INT_MAX;
            break;
        case '?':
            min = 0;
            max = 1;
            break;
        case '{':
            numbers = FindNumber(lex);
            if (numbers.size() != 2) {
                throw logic_error(
                        lex + ":cannot find two numbers between '{' and '}'");
            }
            min = numbers[0];
            max = numbers[1];
            if (min > max) {
                throw logic_error(
                        lex + ":invalid limits min--" + to_string(min) +
                        " max--" + to_string(max));
            }
            break;
    }
    // x?
    if (lex.size() == 2) {
        greedy = false;
    } else {
        greedy = true;
    }
}

shared_ptr<NestedNode> NestedNode::MakeNestedNode(const string &lex) {
    if (lex[1] == '?') {
        switch (lex[2]) {
            case ':':
                return make_shared<CaptureNode>(lex);
            case '=':
            case '!':
                return make_shared<AssertionNode>(lex);
            case '<':
                switch (lex[3]) {
                    case '=':
                    case '!':
                        return make_shared<AssertionNode>(lex);
                    default:
                        return make_shared<CaptureNode>(lex);
                }
            default:
                throw logic_error(lex + ":unknown capture groups syntax");
        }
    } else {
        return make_shared<CaptureNode>(lex);
    }
}

CaptureNode::CaptureNode(const string &lex) : NestedNode(lex) {
    if (lex[1] == '?') {
        auto begin = lex.cbegin() + 3, end = lex.cend();
        switch (lex[2]) {
            case ':':
                // (?:Regex)
                captured = false;
                anonymous = false;
                name = "";
                nested_regex = string(begin, lex.cend() - 1);
                break;
            case '<':
                // (?<name>regex)
                end = begin;
                while (end != lex.cend() && *end != '>') {
                    ++end;
                }
                if (end == lex.cend()) {
                    // lack of '>'
                    throw logic_error(lex + ":lack of '>' in named capture "
                                            "groups");
                }
                captured = true;
                anonymous = false;
                name = string(begin, end);
                nested_regex = string(end + 1, lex.cend());
                break;
            default:
                // syntax error
                throw logic_error(lex + ":unknown capture groups syntax");
        }
    } else {
        // (Regex)
        captured = true;
        anonymous = true;
        name = "";
        nested_regex = string(lex.cbegin() + 1, lex.cend() - 1);
    }
}

AssertionNode::AssertionNode(const string &lex) : NestedNode(lex) {
    switch (lex[2]) {
        case '!':
            nested_regex = string(lex.cbegin() + 3, lex.cend() - 1);
            direction = true;
            matched = false;
            break;
        case '=':
            nested_regex = string(lex.cbegin() + 3, lex.cend() - 1);
            direction = true;
            matched = true;
            break;
        case '<':
            nested_regex = string(lex.cbegin() + 4, lex.cend() - 1);
            direction = false;
            if (lex[3] == '=') {
                matched = true;
            } else {
                matched = false;
            }
            break;
    }
}

vector<int> FindNumber(const string &s) {
    auto begin = s.cbegin(), end = begin;
    vector<int> numbers;
    while (end != s.cend()) {
        while (begin != s.cend() && !isdigit(*begin)) {
            begin++;
        }
        if (begin == s.cend()) {
            return numbers;
        }
        end = begin;
        while (end != s.cend() && isdigit(*end)) {
            end++;
        }
        if (end == s.cend()) {
            return numbers;
        }
        numbers.push_back(stoi(string(begin, end)));
        begin = end + 1;
    }
    return numbers;
}
