//
// Created by dxy on 2020/9/17.
//

#ifndef XYREGENGINE_XY_REGEX_H
#define XYREGENGINE_XY_REGEX_H

#include "nfa.h"

namespace XyRegEngine {
    class Regex;

    class RegexResult {
        friend class Regex;

    public:
        [[nodiscard]] SubMatch GetResult() const {
            return result_;
        }

        [[nodiscard]] std::vector<SubMatch> GetSubMatches() const {
            return sub_matches_;
        }

    private:
        SubMatch result_;  // store the string that matches the given regex
        // All sub-matches are stored in sequence.
        std::vector<SubMatch> sub_matches_;
    };

    class Regex {
    public:
        explicit Regex(const std::string &regex) : nfa_(regex) {}

        /**
         * Determine whether s matches the regex.
         *
         * @param s
         * @param result store detailed result
         * @return
         */
        bool Match(const std::string &s, RegexResult &result);

        /**
         * Determine whether a sub-string in s matches the regex.
         *
         * @param s
         * @param result store detailed result
         * @return
         */
        bool Search(const std::string &s, RegexResult &result);

    private:
        Nfa nfa_;
    };
}

#endif //XYREGENGINE_XY_REGEX_H
