//
// Created by dxy on 2020/8/6.
//

#ifndef XYREGENGINE_LEX_H
#define XYREGENGINE_LEX_H

#include <iterator>
#include <string>

namespace XyRegEngine {
    using StrConstIt = std::string::const_iterator;

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
    std::string NextToken(StrConstIt &begin, StrConstIt &end);

    /**
     * @param begin
     * @param end
     * @return Return the iterator of the first character after the escape
     * characters. If no valid escape characters are found, return begin.
     */
    StrConstIt SkipEscapeCharacters(StrConstIt begin, StrConstIt end);
}

#endif // XYREGENGINE_LEX_H
