//
// Created by dxy on 2020/8/6.
//

#include "lex.h"

#include <map>
#include <string>

using namespace XyRegEngine;
using namespace std;

/**
 * Move 'begin' to the next character after the escape character.
 *
 * @param begin
 * @param end
 * @return Escape character. If no valid escape character exists, it returns "".
 */
string SkipEscapeCharacter(StrConstIt &begin, StrConstIt &end);

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
                if (++begin != end) {
                    return string(cur_begin, ++begin);
                }
                return "";  // lack of escape character

                // See '[...]' '{...}' '<...>' as a lex. Although it cannot
                // be nested, nested error is detected by caller and now we
                // only find the first matched right bracket.
            case '[':
            case '{':
            case '<':
                while (++begin != end) {
                    SkipEscapeCharacter(begin, end);  // skip '\]' '\}' '\>'
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
                    SkipEscapeCharacter(begin, end);
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
            case '>':
            case ')':
                return "";
            default:
                return string(1, *begin++);
        }
    }
    return "";  // All characters in 'regex' have been scanned.
}

string SkipEscapeCharacter(StrConstIt &begin, StrConstIt &end) {
    if (begin != end && *begin == '\\') {
        auto cur_begin = begin;
        if (++begin != end) {
            return string(cur_begin, ++begin);
        }
        return "";  // no characters behind '\\'
    }
    return "";
}