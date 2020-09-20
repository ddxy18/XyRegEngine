//
// Created by dxy on 2020/8/6.
//

#include "lex.h"

#include <map>
#include <string>

using namespace XyRegEngine;
using namespace std;

string XyRegEngine::NextToken(StrConstIt &begin, StrConstIt &end) {
    if (begin != end) {
        auto cur_begin = begin;
        map<char, char> pair_characters{
                {'}', '{'},
                {']', '['}
        };

        // count '(' to ensure correct matches of nested '()'
        int parentheses = 1;
        switch (*begin) {
            case '|':
            case '.':
            case '^':
            case '$':
                return string(1, *begin++);

                // repeat(greedy and non-greedy)
            case '*':
            case '+':
            case '?':
                if (++begin != end) {
                    if (*begin == '?') {
                        return string(cur_begin, ++begin);
                    }
                }
                return string(cur_begin, begin);

            case '\\':  // escape characters
                begin = SkipEscapeCharacters(begin, end);
                return string(cur_begin, begin);

                // See '[...]' '{...}' '<...>' as a lex. Although it cannot
                // be nested, nested error is detected by caller and now we
                // only find the first matched right bracket.
            case '[':
            case '{':
                while (++begin != end) {
                    // skip '\]' '\}'
                    begin = SkipEscapeCharacters(begin, end);
                    if (begin != end) {
                        auto it = pair_characters.find(*begin);
                        if (it != pair_characters.end() &&
                            it->second == *cur_begin) {
                            return string(cur_begin, ++begin);
                        }
                    }
                }
                return "";  // lack of ']' '}' '>'

                // '()' and contents between them are seen as one token and it
                // can be nested.
            case '(':
                while (parentheses != 0 && ++begin != end) {
                    begin = SkipEscapeCharacters(begin, end);
                    if (begin != end) {
                        switch (*begin) {
                            case '(':
                                parentheses++;
                                break;
                            case ')':
                                parentheses--;
                                break;
                        }
                    }
                }
                if (parentheses == 0) {
                    return string(cur_begin, ++begin);
                }
                return "";  // lack of ')'

                // lack of their left pairs
            case ']':
            case '}':
            case ')':
                return "";
            default:
                return string(1, *begin++);
        }
    }
    return "";  // All characters in 'regex' have been scanned.
}

StrConstIt XyRegEngine::SkipEscapeCharacters(StrConstIt begin, StrConstIt end) {
    auto cur_it = begin;

    if (cur_it != end && *cur_it == '\\') {
        cur_it++;

        // back reference
        while (cur_it != end && isdigit(*cur_it)) {
            cur_it++;
        }
        if (cur_it != begin + 1) {
            return cur_it;
        }

        switch (*cur_it) {
            case 'u':
                return cur_it + 5;
            case 'c':
                return cur_it + 2;
            case 'x':
                return cur_it + 3;
            case '\0':
                return begin;
            default:
                return cur_it + 1;
        }
    }

    return begin;  // not escape characters
}