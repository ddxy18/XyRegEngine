//
// Created by dxy on 2020/8/6.
//

#ifndef XYREGENGINE_NFA_H
#define XYREGENGINE_NFA_H

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace XyRegEngine {
    class AstNode;

    class AssertionNfa;

    class GroupNfa;

    class SpecialPatternNfa;

    class RangeNfa;

    using AstNodePtr = std::unique_ptr<AstNode>;
    using StrConstIt = std::string::const_iterator;
    // a sub-match [pair.first, pair.second)
    using SubMatch = std::pair<StrConstIt, StrConstIt>;
    // pair.first -- state
    // pair.second -- current begin iterator
    // map.second -- Sub-matches. Every vector<SubMatch> stores a possible
    // sub-match order.
    using ReachableStatesMap =
    std::map<std::pair<int, StrConstIt>, std::vector<SubMatch>>;
    using State =
    std::pair<std::pair<int, StrConstIt>, std::vector<SubMatch>>;

    enum class Encoding {
        kAscii, kUtf8
    };

    /**
     * We split a regex to several parts and classify them to types in
     * RegexPart.
     *
     * kAnd('&': it is implicit in the regex)
     * kChar(including [...], escape character and single character)
     * kQuantifier('*' '?' '+' and their non-greedy mode, '{...}')
     * kAlternative('|')
     * kCapture('(...)' or '(?:...)')
     * kAssertion('(?=...)', '(?!...)', '^', '$', '\b', '\B')
     * kError(invalid token)
     */
    enum class RegexPart {
        kAnd, kChar, kQuantifier, kAlternative, kGroup, kAssertion, kError
    };

    class Nfa {
        friend class NfaFactory;

        friend class AssertionNfa;

    public:
        /**
         * Build a NFA for 'regex'. Notice that if 'regex' is invalid, it
         * creates an empty NFA.
         *
         * @param regex
         */
        explicit Nfa(const std::string &regex);

        /**
         * It is used to determine whether a NFA is a valid NFA.
         *
         * @return
         */
        [[nodiscard]] bool Empty() const {
            // all invalid NFAs should have accept_state_ set as -1
            return accept_state_ == -1;
        }

        /**
         * Get the next match in a given string in the range of [begin, end).
         * Notice that it matches from begin, say, the successful match must
         * be [begin, any location no more than end).
         *
         * @param begin First iterator of the given string. If we successfully
         * find a token, 'begin' will be moved to the beginning of the first
         * character after the match.
         * @param end Last iterator of the given string.
         * @return A matched substring. If no match exists, it returns "".
         */
        std::string NextMatch(StrConstIt &begin, StrConstIt end);

    protected:
        enum class StateType {
            kAssertion, kGroup, kSpecialPattern, kRange, kCommon
        };

        static const int kEmptyEdge = 0;

        Nfa() = default;

        /**
         * Build a NFA according to an AST. If ast_head is an invalid AST or
         * a nullptr pointer, it creates an empty NFA.
         *
         * @param ast_head can be nullptr
         */
        Nfa(AstNodePtr &ast_head, const std::vector<unsigned int> &char_ranges);

        /**
         * It copies exchange_map_, assertion_states_, not_assertion_states_
         * and group_states_ to the new NFA.
         *
         * @param nfa After calling the function, nfa.exchange_map_ becomes
         * undefined and should never be accessed again.
         * @return
         */
        Nfa &operator+=(Nfa &nfa);

        /**
         * It tries to move to the next state until to the end of the string or
         * no next states can be found.
         *
         * @param begin
         * @param end
         * @return all possible routines
         */
        std::vector<ReachableStatesMap> StateRoute(
                StrConstIt begin, StrConstIt end);

        /**
         * Get all reachable states starting from cur_state. When cur_state
         * is a functional state, it will be handled first. For common
         * states, we match *begin first. Then we deal with states that can
         * be reached through empty edges.
         *
         * @param cur_state
         * @param str_begin
         * @param str_end
         * @return reachable states after handling cur_state
         */
        ReachableStatesMap
        NextState(const State &cur_state, StrConstIt str_begin,
                  StrConstIt str_end);

        /**
         * All reachable states starting from cur_state through empty edges.
         * Notice that cur_state is excluded and it does nothing to handle
         * functional states.
         *
         * @param cur_state
         * @return
         */
        ReachableStatesMap NextState(const State &cur_state);

        StateType GetStateType(int state);

        /**
         * Use 'delim' to split an encoding to several ranges. By default,
         * every range in an alphabet table is a character.
         *
         * @param delim You should ensure it contains valid character classes.
         * An empty vector means default split mode.
         * @param encoding
         */
        void CharRangesInit(
                const std::vector<std::string> &delim, Encoding encoding);

        /**
         * Find which character range in the char_ranges_ c is in.
         *
         * @param c must be in the range of the current encoding
         * @return range index in char_ranges_ where c is in
         */
        int GetCharLocation(int c);

        /**
         * Parse a regex to an AST. We assume that regex can only include
         * several valid elements:
         * characters(include single character, escape character and '[...]')
         * quantifiers
         * |
         * & (It is implicit in a regex string but we need to explicitly deal
         * with them when parsing a regex)
         * groups
         * assertions
         *
         * All characters should be represented as a node, say, ranges and
         * escape characters are seen as single character here. Moreover, groups
         * and lookaheads will add a flag node to the head to remember its type.
         *
         * @param regex
         * @return If regex is in a valid format, return the head of AST.
         * Otherwise return nullptr.
         */
        static AstNodePtr ParseRegex(const std::string &regex);

        /**
         * Add a new state to exchange_map_. Now it has no edges.
         */
        void NewState();

        /**
         * We use i_ to generate a new state. First state should have a id 1.
         * After that, it will be increased when creating a new state. i_
         * points to the lastly added state at any time after creating the
         * first state.
         */
        static int i_;

        /**
         * Record several continuous character ranges. Ranges are stored
         * orderly. Range i refers to [char_ranges_[i], char_ranges_[i + 1]).
         * Since characters with code of 0-4 are almost never appears in
         * text, we use them to represent special edges. And char_ranges_
         * should coverage every character in the given encoding.
         */
        std::vector<unsigned int> char_ranges_;

        /**
         * map.first -- state
         * map.second.element -- reachable states from an input character that
         * is in the corresponding range
         */
        std::map<int, std::vector<std::set<int>>> exchange_map_;

        /**
         * We compress an assertion to an assertion state in the NFA and it is
         * connected to other states through empty edges. So we need a map to
         * connect the assertion state to the corresponding assertion NFA.
         */
        std::map<int, AssertionNfa> assertion_states_;

        std::map<int, GroupNfa> group_states_;

        std::map<int, SpecialPatternNfa> special_pattern_states_;

        std::map<int, RangeNfa> range_states_;

        int begin_state_{-1};
        int accept_state_{-1};
    };

    /**
     * ^ $ \\b \\B (?=sub-pattern) (?!sub-pattern)
     */
    class AssertionNfa {
    public:
        AssertionNfa(const AssertionNfa &assertion_nfa) = default;

        explicit AssertionNfa(const std::string &assertion);

        /**
         * @param str_begin location of ^ in regex in a match
         * @param str_end location of $ in regex in a match
         * @param begin where to start the assertion
         * @return
         */
        bool IsSuccess(StrConstIt str_begin, StrConstIt str_end,
                       StrConstIt begin);

    private:
        enum class AssertionType {
            kLineBegin, kLineEnd, kWordBoundary, kNotWordBoundary,
            kPositiveLookahead, kNegativeLookahead
        };

        AssertionType type_;

        Nfa nfa_;
    };

    /**
     * It stores a NFA for the sub-pattern in the group.
     */
    class GroupNfa : public Nfa {
    public:
        GroupNfa(const GroupNfa &group_nfa) = default;

        explicit GroupNfa(const std::string &regex) : Nfa(regex) {}

        /**
         * Get all possible sub-matches from the group.
         *
         * @param begin
         * @param str_end
         * @return possible end iterators after dealing with the group
         */
        std::set<StrConstIt> NextMatch(StrConstIt begin, StrConstIt str_end);
    };

    /**
     * Escape characters, special meaning escape characters and back-reference.
     */
    class SpecialPatternNfa {
    public:
        explicit SpecialPatternNfa(std::string characters) :
                characters_(std::move(characters)) {}

        /**
         * Determine whether a substring can match characters_.
         *
         * @param begin
         * @param str_end
         * @return If a substring [begin, end_it) matches, return end_it.
         * Otherwise return begin.
         */
        StrConstIt NextMatch(const State &state, StrConstIt str_end);

    private:
        std::string characters_;
    };

    /**
     * [...]. It can contain single characters, ranges and special patterns.
     */
    class RangeNfa {
    public:
        explicit RangeNfa(const std::string &regex);

        /**
         * @param begin
         * @param str_end
         * @return If a substring [begin, end_it) matches, return end_it.
         * Otherwise return begin.
         */
        StrConstIt NextMatch(const State &state, StrConstIt str_end);

    private:
        std::map<int, int> ranges_;
        std::vector<SpecialPatternNfa> special_patterns_;
    };

    /**
     * Provide a function to create a sub-NFA for common operators in the
     * RegexPart. It's usually used to create NFAs for split parts.
     */
    class NfaFactory {
    public:
        static Nfa MakeCharacterNfa(
                const std::string &characters,
                const std::vector<unsigned int> &char_ranges);

        static Nfa MakeAlternativeNfa(Nfa left_nfa, Nfa right_nfa);

        static Nfa MakeAndNfa(Nfa left_nfa, Nfa right_nfa);

        static Nfa MakeQuantifierNfa(const std::string &quantifier,
                                     AstNodePtr &left,
                                     const std::vector<unsigned int> &char_ranges);

    private:
        static std::pair<int, int> ParseQuantifier(
                const std::string &quantifier);
    };

    /**
     * Node in AST that is used as a intermediate format of a regex. It helps
     * to show the regex structure more clearly and reduce the work to build
     * the NFA. AST for regex is designed as a binary tree, so every AstNode
     * has at most two son nodes.
     */
    class AstNode {
        friend class Nfa;

    public:
        AstNode(RegexPart regex_type, std::string regex) :
                regex_type_(regex_type), regex_(std::move(regex)) {}

        void SetLeftSon(AstNodePtr left_son) {
            left_son_ = std::move(left_son);
        }

        void SetRightSon(AstNodePtr right_son) {
            right_son_ = std::move(right_son);
        }

        [[nodiscard]] RegexPart GetType() const {
            return regex_type_;
        }

    private:
        RegexPart regex_type_;
        std::string regex_;
        AstNodePtr left_son_;
        AstNodePtr right_son_;
    };
}

#endif // XYREGENGINE_NFA_H