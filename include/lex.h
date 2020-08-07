//
// Created by dxy on 2020/8/6.
//

#ifndef XYREGENGINE_LEX_H
#define XYREGENGINE_LEX_H

#include <string>
#include <iterator>

/**
 * It finds one token a time and sets 'begin' to the head of next token.
 * Notice that '(...)' '{...}' '<...>' '[...]' are seen as a token.
 *
 * @param regex
 * @param begin Always point to the begin of the next token. If no valid
 * token remains, it points to 'iterator.end()'.
 * @param end It isn't modified during execution.
 * @return std::string A valid token. If finding invalid token or reaching
 * to the end of the 'regex', it returns "".
 */
std::string
NextToken(const std::string &regex, std::string::const_iterator &begin,
          std::string::const_iterator &end);

#endif //XYREGENGINE_LEX_H
