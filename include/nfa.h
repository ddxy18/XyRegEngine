//
// Created by dxy on 2020/8/6.
//

#ifndef XYREGENGINE_NFA_H
#define XYREGENGINE_NFA_H

#include <map>
#include <memory>
#include <ostream>
#include <set>
#include <stack>
#include <string>
#include <vector>

namespace XyRegEngine {
    class AstNode;

    using AstNodePtr = std::unique_ptr<AstNode>;

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

        using StrConstIt = std::string::const_iterator;

    public:
        Nfa() = default;

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

    private:
        static const int kEmptyEdge = 0;
        static const int kLineBeginEdge = 1;  // ^
        static const int kLineEndEdge = 2;  // $
        static const int kWordBoundaryEdge = 3;  // \b
        static const int kNotWordBoundaryEdge = 4;  // \B

        explicit Nfa(std::vector<unsigned int> char_ranges) :
                char_ranges_(std::move(char_ranges)) {}

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
         * Get all reachable states starting from cur_state through *begin.
         * *begin should be matched firstly before matching any other edges.
         * States that can be accessed through empty edges from currently
         * matched states should also be marked reachable.
         *
         * @param cur_state
         * @param begin
         * @param end
         * @return pair.first.element -- reachable common states
         *         pair.second.element -- reachable function states
         */
        std::pair<std::set<int>, std::set<int>> NextState(
                int cur_state, StrConstIt begin, StrConstIt end);

        /**
         * All reachable states starting from cur_state through empty edges,
         * cur_state is included in pair.first as well.
         *
         * @param cur_state
         * @return pair.first.element -- reachable common states
         *         pair.second.element -- reachable function states
         */
        std::pair<std::set<int>, std::set<int>> NextState(int cur_state);

        /**
         * Handle an assertion part in the NFA.
         *
         * @param head_state an assertion head state
         * @param begin
         * @param end
         * @param type true -- positive lookahead
         *             false -- negative lookahead
         * @return If assertion succeeds, return all reachable states from the
         * tail state. Otherwise pair.first and pair.second should be empty.
         */
        std::pair<std::set<int>, std::set<int>> CheckAssertion(
                int head_state, StrConstIt begin, StrConstIt end, bool type);

        /**
         * Handle any assertion head states existed in func_states. It
         * automatically handles continuous assertions until no assertion
         * head states are reachable.
         *
         * @param func_states
         * @param begin
         * @param end
         * @return Contain group states in func_states and other reachable
         * states after completing all assertions.
         */
        std::pair<std::set<int>, std::set<int>> CheckAssertion(
                std::set<int> &func_states, StrConstIt begin, StrConstIt end);

        bool IsFuncState(int state);

        /**
         * Determine function head state's type.
         *
         * @param state
         * @return 0 -- positive lookahead and special assertions
         *         1 -- negative lookahead
         *         2 -- group
         *         -1 -- common state and function tail state
         */
        int GetStateType(int state);

        /**
         * Get the corresponding tail state of head_state
         *
         * @param head_state
         * @return
         */
        int GetTailState(int head_state);

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
         * pair.first -- head state
         * pair.second -- corresponding tail state
         */
        std::set<std::pair<int, int>> assertion_states_;
        std::set<std::pair<int, int>> not_assertion_states_;
        std::set<std::pair<int, int>> group_states_;

        int begin_state_{-1};
        int accept_state_{-1};
    };

    /**
     * Provide a function to create a NFA for every type in the RegexPart. It's
     * usually used to create NFAs for split parts.
     */
    class NfaFactory {
    public:
        static Nfa MakeCharacterNfa(
                std::string characters, std::vector<unsigned int> char_ranges);

        static Nfa MakeAlternativeNfa(Nfa left_nfa, Nfa right_nfa);

        static Nfa MakeAndNfa(Nfa left_nfa, Nfa right_nfa);

        static Nfa MakeQuantifierNfa(const std::string &quantifier,
                                     AstNodePtr &left,
                                     std::vector<unsigned int> char_ranges);

        static Nfa MakeGroupNfa(std::string group, Nfa left_nfa);

        static Nfa MakeAssertionNfa(std::string assertion, Nfa left_nfa);

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

#endif //XYREGENGINE_NFA_H