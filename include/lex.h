//
// Created by dxy on 2020/10/18.
//

#ifndef XYREGENGINE_LEX_H
#define XYREGENGINE_LEX_H

namespace XyRegEngine {
// We use integer constants to replace string literals, so we can use a
// common representation for literals.
const int kReverseSolidus = 0x5c;  // '\'
const int kFullStop = 0x2e;  // .
const int kAsterisk = 0x2a;  // *
const int kPlusSign = 0x2b;  // +
const int kQuestionMark = 0x3f;  // ?
const int kDollarSign = 0x24;  // $
const int kLeftParenthesis = 0x28;  // (
const int kRightParenthesis = 0x29;  // )
const int kExclamationMark = 0x21;  // !
const int kCircumflexAccent = 0x5e;  // ^
const int kNull = 0x0;  // \0
const int kHorizontalTab = 0x09;  // \t
const int kLineFeed = 0x0a;  // \n
const int kVerticalTab = 0x0b;  // \v
const int kFormFeed = 0x0c;  // \f
const int kCarriageReturn = 0x0d;  // \r

template<class T>
using StrConstIt = typename std::basic_string<T>::const_iterator;

template<class T>
std::basic_string<T> NextToken(StrConstIt<T> &begin, StrConstIt<T> &end) {
  using namespace std;

  if (begin != end) {
    auto cur_begin = begin;
    std::map<char, char> pair_characters{
            {'}', '{'},
            {']', '['}
    };

    // count ( to ensure correct matches of nested ()
    int parentheses = 1;
    switch (*begin) {
      case '|':
      case '.':
      case '^':
      case '$':
        return basic_string<T>(1, *begin++);

        // repeat(greedy and non-greedy)
      case '*':
      case '+':
      case '?':
        if (++begin != end) {
          if (*begin == '?') {
            return basic_string<T>(cur_begin, ++begin);
          }
        }
        return basic_string<T>(cur_begin, begin);

      case '\\':  // escape characters
        begin = SkipEscapeCharacters<T>(begin, end);
        return basic_string<T>(cur_begin, begin);

        // See [...] {...} <...> as a lex. Although it cannot be nested,
        // nested error is detected by caller and now we only find the first
        // matched right bracket.
      case '[':
      case '{':
        while (++begin != end) {
          // skip \] \}
          begin = SkipEscapeCharacters<T>(begin, end);
          if (begin != end) {
            auto it = pair_characters.find(*begin);
            if (it != pair_characters.end() &&
                it->second == *cur_begin) {
              return basic_string<T>(cur_begin, ++begin);
            }
          }
        }
        return basic_string<T>();  // lack of ] } >

        // () and contents between them are seen as one token and it can be
        // nested.
      case '(':
        while (parentheses != 0 && ++begin != end) {
          begin = SkipEscapeCharacters<T>(begin, end);
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
          return basic_string<T>(cur_begin, ++begin);
        }
        return basic_string<T>();  // lack of )

        // lack of their left pairs
      case ']':
      case '}':
      case ')':
        return basic_string<T>();
      default:
        return basic_string<T>(1, *begin++);
    }
  }
  return basic_string<T>();  // All characters in regex have been scanned.
}

template<class T>
StrConstIt<T> SkipEscapeCharacters(StrConstIt<T> begin, StrConstIt<T> end) {
  using namespace std;

  auto cur_it = begin;

  if (cur_it != end && *cur_it == basic_string<T>(1, kReverseSolidus)[0]) {
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
}

#endif //XYREGENGINE_LEX_H
