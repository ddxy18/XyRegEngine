//
// Created by dxy on 2020/8/6.
//

#include "include/lex.h"

#include <map>
#include <string>

using namespace std;

string NextToken(const string &regex, string::const_iterator &begin,
                 string::const_iterator &end) {
    if (begin != end) {
        auto cur_begin = begin;
        map<char, char> bracket{
                {'}', '{'},
                {']', '['},
                {'>', '<'}
        };

        int bracket_num = 1;
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
                        begin++;
                        return string(begin - 2, begin);
                    }
                }
                return string(begin - 1, begin);

                // escape characters
            case '\\':
                if (++begin != end) {
                    begin++;
                    return string(begin - 2, begin);
                }
                return "";

                /**
                 * See '[...]' '{...}' '<...>' as a lex. Although it cannot
                 * be nested, nested error is detected by caller and now
                 * we only find the first matched right bracket.
                 */
            case '[':
            case '{':
            case '<':
                cur_begin = begin;
                while (++begin != end) {
                    auto it = bracket.find(*begin);
                    if (it != bracket.end() && it->second == *cur_begin) {
                        return string(cur_begin, ++begin);
                    }
                }
                // lack of ']' '}' '>'
                return "";

                /**
                 * '()' and contents between them are seen as one token and it 
                 * can be nested.
                 */
            case '(':
                cur_begin = begin;
                while (bracket_num != 0 && ++begin != end) {
                    switch (*begin) {
                        case '(':
                            bracket_num++;
                            break;
                        case ')':
                            bracket_num--;
                            break;
                    }
                }
                if (bracket_num == 0) {
                    begin++;
                    return string(cur_begin, begin);
                }
                // lack of ')'
                return "";
            default:
                return string(1,*begin++);
        }
    }
    // All characters in 'regex' have been scanned.
    return "";
}