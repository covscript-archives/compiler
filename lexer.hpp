//
// Created by kiva on 2020/3/10.
//
#pragma once

#include <stack>
#include <deque>
#include <memory>
#include <unordered_map>
#include <mozart++/string>
#include <mozart++/iterator_range>
#include <mozart++/format>
#include <utility>

namespace cs_impl {
    enum class token_type {
        UNDEFINED,
        ID_OR_KW,
        INT_LITERAL,
        FLOATING_LITERAL,
        STRING_LITERAL,
        PREPROCESSOR,
        OPERATOR,
        CUSTOM_LITERAL,
    };

    enum class operator_type {
        UNDEFINED,
        OPERATOR_ADD,
        OPERATOR_SUB,
        OPERATOR_MUL,
        OPERATOR_DIV,
        OPERATOR_MOD,
        OPERATOR_ASSIGN,
        OPERATOR_ADD_ASSIGN,
        OPERATOR_SUB_ASSIGN,
        OPERATOR_MUL_ASSIGN,
        OPERATOR_DIV_ASSIGN,
        OPERATOR_MOD_ASSIGN,
        OPERATOR_AND_ASSIGN,
        OPERATOR_OR_ASSIGN,
        OPERATOR_XOR_ASSIGN,
        OPERATOR_EQ,
        OPERATOR_NE,
        OPERATOR_GT,
        OPERATOR_GE,
        OPERATOR_LT,
        OPERATOR_LE,
        OPERATOR_COLON,
        OPERATOR_COMMA,
        OPERATOR_QUESTION,
        OPERATOR_INC,
        OPERATOR_DEC,
        OPERATOR_ARROW,
        OPERATOR_DOT,
        OPERATOR_AND,
        OPERATOR_OR,
        OPERATOR_NOT,
        OPERATOR_BITAND,
        OPERATOR_BITOR,
        OPERATOR_BITXOR,
        OPERATOR_BITNOT,
        OPERATOR_VARARG,
        OPERATOR_LPAREN,   // (
        OPERATOR_RPAREN,   // )
        OPERATOR_LBRACKET, // [
        OPERATOR_RBRACKET, // ]
        OPERATOR_LBRACE,   // {
        OPERATOR_RBRACE,   // }

        OPERATOR_SEMI,
    };

    struct token {
        std::size_t _line;
        std::size_t _column;
        std::string _token_text;
        token_type _type;

        explicit token(std::size_t line, std::size_t column,
                       std::string text, token_type type)
            : _line(line), _column(column),
              _token_text(std::move(text)), _type(type) {}

        virtual ~token() = default;
    };

    struct token_operator : public token {
        std::string _value;
        operator_type _op_type;

        explicit token_operator(std::size_t line, std::size_t column,
                                std::string text, std::string value, operator_type type)
            : token(line, column, std::move(text), token_type::OPERATOR),
              _value(std::move(value)), _op_type(type) {}

        ~token_operator() override = default;
    };

    struct token_preprocessor : public token {
        std::string _value;

        explicit token_preprocessor(std::size_t line, std::size_t column,
                                    std::string text, std::string value)
            : token(line, column, std::move(text), token_type::PREPROCESSOR),
              _value(std::move(value)) {}

        ~token_preprocessor() override = default;
    };

    struct token_id_or_kw : public token {
        std::string _value;

        explicit token_id_or_kw(std::size_t line, std::size_t column,
                                std::string text, std::string value)
            : token(line, column, std::move(text), token_type::ID_OR_KW),
              _value(std::move(value)) {}

        ~token_id_or_kw() override = default;
    };

    struct token_int_literal : public token {
        int64_t _value;

        explicit token_int_literal(std::size_t line, std::size_t column,
                                   std::string text, int64_t value)
            : token(line, column, std::move(text), token_type::INT_LITERAL),
              _value(value) {}

        ~token_int_literal() override = default;
    };

    struct token_float_literal : public token {
        double _value;

        explicit token_float_literal(std::size_t line, std::size_t column,
                                     std::string text, double value)
            : token(line, column, std::move(text), token_type::FLOATING_LITERAL),
              _value(value) {}

        ~token_float_literal() override = default;
    };

    struct token_string_literal : public token {
        std::string _value;

        explicit token_string_literal(std::size_t line, std::size_t column,
                                      std::string text, std::string value)
            : token(line, column, std::move(text), token_type::STRING_LITERAL),
              _value(std::move(value)) {}

        ~token_string_literal() override = default;
    };

    struct token_custom_literal : public token {
        std::unique_ptr<token> _literal;
        std::string _suffix;

        explicit token_custom_literal(std::size_t line, std::size_t column,
                                      std::string text, std::unique_ptr<token> literal,
                                      std::string value)
            : token(line, column, std::move(text), token_type::CUSTOM_LITERAL),
              _literal(std::move(literal)),
              _suffix(std::move(value)) {}

        ~token_custom_literal() override = default;
    };

    enum class lexer_state {
        GLOBAL,
        INT_LIT,
        FLOATING_LIT,
        STRING_LIT,
        PREPROCESSOR,
        OPERATOR,
        LITERAL_SUFFIX,

        PARSING_STRING,
        TRYING_LITERAL_SUFFIX,

        ERROR_EOF,
        ERROR_ESCAPE,
        ERROR_OPERATOR,
    };

    struct state_manager {
    private:
        lexer_state _state = lexer_state::GLOBAL;
        std::stack<lexer_state> _previous;

        lexer_state last_state() {
            if (_previous.empty()) {
                return lexer_state::GLOBAL;
            }
            lexer_state state = _previous.top();
            _previous.pop();
            return state;
        }

    public:
        void new_state(lexer_state state) {
            _previous.push(_state);
            _state = state;
        }

        void end(lexer_state expected) {
            if (_state == expected) {
                _state = last_state();
            }
        }

        void replace(lexer_state state) {
            _state = state;
        }

        lexer_state pop() {
            lexer_state state = current();
            end(state);
            return state;
        }

        lexer_state current() const {
            return _state;
        }
    };

    template <typename CharT>
    struct lexer_input {
    private:
        const CharT *_begin = nullptr;
        std::size_t _length = 0;

    public:
        void source(const CharT *begin, std::size_t length) {
            this->_begin = begin;
            this->_length = length;
        }

        mpp::iterator_range<CharT> ranges() const {
            return mpp::make_range(_begin, _begin + _length);
        }

        const CharT *begin() const {
            return _begin;
        }

        const CharT *end() const {
            return _begin + _length;
        }
    };

    struct lexer_error : public std::runtime_error {
        std::size_t _line;
        std::size_t _column;
        std::string _error_text;

        explicit lexer_error(std::size_t line, std::size_t column,
                             std::string error_text,
                             const std::string &message)
            : std::runtime_error(message), _line(line), _column(column),
              _error_text(std::move(error_text)) {
        }

        ~lexer_error() override = default;
    };

    template <typename CharT>
    struct basic_lexer {
        using iter_t = const CharT *;
    private:
        state_manager _state;
        lexer_input<CharT> _input;
        std::unordered_map<std::string, operator_type> _op_maps;

        template <typename T, typename ...Args>
        std::unique_ptr<token> make_token(std::size_t line, iter_t line_start,
                                          iter_t token_start, iter_t token_end,
                                          Args &&...args) {
            return std::unique_ptr<token>(
                new T{line, static_cast<std::size_t>(token_start - line_start),
                      std::basic_string<CharT>(token_start, token_end - token_start),
                      std::forward<Args>(args)...}
            );
        }

        template <typename ...Args>
        __attribute__((noreturn))
        void error(std::size_t line, iter_t line_start,
                   iter_t token_start, iter_t token_end,
                   const std::string &fmt, Args &&...args) {
            auto message = mpp::format(fmt, std::forward<Args>(args)...);
            mpp::throw_ex<lexer_error>(
                line, static_cast<std::size_t>(token_end - line_start),
                std::basic_string<CharT>(token_start, token_end - token_start),
                message
            );
            std::terminate();
        }

        // return true if it's id or keyword
        bool is_id_or_kw(CharT c, bool first) const {
            return std::isalpha(c)
                   || c == '$'
                   || c == '_'
                   || (!first && std::isdigit(c));
        }

        bool is_separator_char(CharT c) {
            return std::isspace(c) || c == ';';
        }

        mpp::string_ref consume_preprocessor(iter_t &current, iter_t &end) {
            iter_t left = current;
            mpp::string_ref now(current, end - current);
            std::size_t new_line_start = now.find_first_of('\n');

            _state.new_state(lexer_state::PREPROCESSOR);
            if (new_line_start != mpp::string_ref::npos) {
                current += new_line_start;
                return mpp::string_ref{left, new_line_start};
            } else {
                current = end;
                return mpp::string_ref{left, static_cast<std::size_t>(current - left)};
            }
        }

        std::pair<int64_t, double> consume_number(iter_t &current, iter_t &end) {
            int64_t integer_part = *current++ - '0';
            if (current == end) {
                _state.new_state(lexer_state::INT_LIT);
                return std::make_pair(integer_part, 0);
            }

            // lookahead for (.)
            bool found_point = false;
            iter_t lookahead = current;
            while (lookahead < end) {
                if (*current == '.') {
                    found_point = true;
                    break;
                }
                if (std::isdigit(*lookahead)) {
                    ++lookahead;
                } else {
                    break;
                }
            }

            // starts with non-zero or contains floating point: must be dec
            if (integer_part != 0 || found_point) {
                // parsing dec number
                bool after_point = false;
                int64_t floating_part = 0;
                int npoints = 1;

                while (current < end) {
                    if (std::isdigit(*current)) {
                        if (after_point) {
                            floating_part = floating_part * 10 + *current++ - '0';
                            npoints *= 10;
                        } else {
                            integer_part = integer_part * 10 + *current++ - '0';
                        }
                    } else if (*current == '.') {
                        after_point = true;
                        current++;
                    } else {
                        // consumed the number
                        break;
                    }
                }

                if (after_point) {
                    double value = integer_part + 1.0 * floating_part / npoints;
                    _state.new_state(lexer_state::FLOATING_LIT);
                    return std::make_pair(0, value);
                } else {
                    _state.new_state(lexer_state::INT_LIT);
                    return std::make_pair(integer_part, 0);
                }
            }

            // starts with 0x/0b/0
            // must be int
            _state.new_state(lexer_state::INT_LIT);

            if (*current == 'x' || *current == 'X') {
                // parsing hex number
                ++current;
                while (current < end && ((*current >= '0' && *current <= '9')
                                         || (*current >= 'a' && *current <= 'f')
                                         || (*current >= 'A' && *current <= 'F'))) {
                    integer_part = integer_part * 16
                                   + (*current & 15)
                                   + (*current >= 'A' ? 9 : 0);
                    ++current;
                }

                return std::make_pair(integer_part, 0);

            } else if (*current == 'b' || *current == 'B') {
                // parsing binary number
                ++current;
                while (current < end && (*current == '0' || *current == '1')) {
                    integer_part = integer_part * 2 + *current - '0';
                    ++current;
                }

                return std::make_pair(integer_part, 0);

            } else {
                // parsing oct number
                while (current < end && *current >= '0' && *current <= '7') {
                    integer_part = integer_part * 8 + *current - '0';
                    ++current;
                }

                return std::make_pair(integer_part, 0);
            }
        }

        mpp::string_ref consume_string_lit(iter_t &current, iter_t &end) {
            if (*current == '"') {
                ++current;
            }

            // string start
            iter_t left = current;
            _state.new_state(lexer_state::PARSING_STRING);

            bool escape = false;
            while (current < end && _state.current() == lexer_state::PARSING_STRING) {
                if (escape) {
                    switch (*current) {
                        case 'r':  // \r
                        case 'n':  // \n
                        case 't':  // \t
                        case 'x':  // \x
                        case 'b':  // \b
                        case '\\':
                        case '0':  // \033
                        case 'e':  // \e
                        case '"':  // \"
                            escape = false;
                            ++current;
                            break;
                        default:
                            // invalid escape char
                            _state.replace(lexer_state::ERROR_ESCAPE);
                            break;
                    }
                } else {
                    switch (*current) {
                        case '\\':
                            escape = true;
                            ++current;
                            break;
                        case '"':
                            _state.replace(lexer_state::STRING_LIT);
                            ++current;
                            break;
                        default:
                            ++current;
                            break;
                    }
                }
            }

            switch (_state.current()) {
                case lexer_state::STRING_LIT:
                    return mpp::string_ref{left, static_cast<std::size_t>(current - left - 1)};
                case lexer_state::PARSING_STRING:
                    // unexpected EOF when parsing string
                    _state.replace(lexer_state::ERROR_EOF);
                    // fall-through
                default:
                    // error happened, reason stored in state
                    return mpp::string_ref{};
            }
        }

        mpp::string_ref consume_id_or_kw(iter_t &current, iter_t &end) {
            // start part
            iter_t left = current++;
            while (current < end && is_id_or_kw(*current, false)) {
                ++current;
            }
            return mpp::string_ref{left, static_cast<std::size_t>(current - left)};
        }

        std::pair<mpp::string_ref, operator_type> consume_operator(iter_t &current, iter_t &end) {
            iter_t left = current;
            while (current < end
                   && !is_separator_char(*current)
                   && !is_id_or_kw(*current, false)) {
                ++current;
            }

            mpp::string_ref op{left, static_cast<std::size_t>(current - left)};
            auto iter = _op_maps.find(op.str());
            if (iter != _op_maps.end()) {
                _state.new_state(lexer_state::OPERATOR);
                return std::make_pair(op, iter->second);
            }

            _state.new_state(lexer_state::ERROR_OPERATOR);
            return std::make_pair(op, operator_type::UNDEFINED);
        }

        mpp::string_ref try_consume_literal_suffix(std::deque<std::unique_ptr<token>> &tokens,
                                                   std::size_t line_no, iter_t line_start,
                                                   iter_t &current, iter_t end) {
            // got literal suffix
            if (*current != '_') {
                return mpp::string_ref{};
            }

            auto value = consume_id_or_kw(current, end);
            _state.new_state(lexer_state::LITERAL_SUFFIX);
            return value;
        }

    public:
        void source(const std::basic_string<CharT> &str) {
            _input.source(str.c_str(), str.length());
        }

        void add_operators(const std::unordered_map<std::string, operator_type> &ops) {
            _op_maps.insert(ops.begin(), ops.end());
        }

        void lex(std::deque<std::unique_ptr<token>> &tokens) {
            iter_t p = _input.begin();
            iter_t end = _input.end();

            // current line start position
            std::size_t line_no = 1;
            iter_t line_start = p;

            while (p != end) {
                /////////////////////////////////////////////////////////////////
                // special position
                /////////////////////////////////////////////////////////////////
                // tokens only available in the beginning of a line
                if (line_start == p) {
                    if (*p == '#' || *p == '@') {
                        // comment and preprocessor tag in CovScript 3
                        iter_t token_start = p;
                        auto value = consume_preprocessor(p, end);
                        switch (_state.pop()) {
                            case lexer_state::PREPROCESSOR:
                                tokens.push_back(make_token<token_preprocessor>(line_no, line_start,
                                    token_start, p, value.str()));
                                break;
                            default:
                                error(line_no, line_start, token_start, p,
                                    "<internal error>: illegal state in preprocessor tag");
                        }
                        continue;
                    }
                }

                /////////////////////////////////////////////////////////////////
                // special state
                /////////////////////////////////////////////////////////////////
                if (_state.current() == lexer_state::TRYING_LITERAL_SUFFIX) {
                    // parse custom literals
                    _state.end(lexer_state::TRYING_LITERAL_SUFFIX);

                    // lookahead and parse literal suffix
                    iter_t token_start = p;
                    auto value = try_consume_literal_suffix(tokens, line_no, line_start, p, end);

                    if (_state.current() == lexer_state::LITERAL_SUFFIX) {
                        _state.end(lexer_state::LITERAL_SUFFIX);
                        if (tokens.empty()) {
                            error(line_no, line_start, p, p,
                                "<internal error>: illegal state in literal suffix");
                        }

                        switch (tokens.back()->_type) {
                            case token_type::STRING_LITERAL:
                            case token_type::INT_LITERAL:
                            case token_type::FLOATING_LITERAL:
                                break;
                            default:
                                error(line_no, line_start, token_start, p,
                                    "unsupported literal suffix {} after non-literal", value.str());
                        }

                        auto literal = std::move(tokens.back());
                        tokens.pop_back();
                        tokens.push_back(make_token<token_custom_literal>(
                            line_no, line_start, token_start, p,
                            std::move(literal), value.str()));
                    }
                    continue;
                }

                /////////////////////////////////////////////////////////////////
                // global state
                /////////////////////////////////////////////////////////////////

                // skip separators
                if (is_separator_char(*p)) {
                    ++p;
                    continue;
                }

                // if we meet \n
                if (*p == '\n') {
                    ++line_no;
                    line_start = ++p;
                    continue;
                }

                // digit
                if (std::isdigit(*p)) {
                    iter_t token_start = p;
                    auto result = consume_number(p, end);
                    switch (_state.pop()) {
                        case lexer_state::INT_LIT:
                            tokens.push_back(make_token<token_int_literal>(
                                line_no, line_start, token_start, p, result.first));
                            // try parse literal suffix
                            _state.new_state(lexer_state::TRYING_LITERAL_SUFFIX);
                            break;
                        case lexer_state::FLOATING_LIT:
                            tokens.push_back(make_token<token_float_literal>(
                                line_no, line_start, token_start, p, result.second));
                            // try parse literal suffix
                            _state.new_state(lexer_state::TRYING_LITERAL_SUFFIX);
                            break;
                        default:
                            error(line_no, line_start, token_start, p,
                                "<internal error>: illegal state in number literal");
                    }
                    continue;
                }

                // string literal
                if (*p == '"') {
                    iter_t token_start = p;
                    auto value = consume_string_lit(p, end);
                    switch (_state.pop()) {
                        case lexer_state::STRING_LIT:
                            tokens.push_back(make_token<token_string_literal>(
                                line_no, line_start, token_start, p, value.str()));
                            // try parse literal suffix
                            _state.new_state(lexer_state::TRYING_LITERAL_SUFFIX);
                            break;
                        case lexer_state::ERROR_EOF:
                            printf("unexpected EOF\n");
                            error(line_no, line_start, token_start, p,
                                "unexpected EOF");
                        case lexer_state::ERROR_ESCAPE:
                            error(line_no, line_start, token_start, p,
                                "unsupported escape char: \\{}", *p);
                        default:
                            error(line_no, line_start, token_start, p,
                                "<internal error>: illegal state in string literal");
                    }
                    continue;
                }

                // id or kw
                if (is_id_or_kw(*p, true)) {
                    iter_t token_start = p;
                    auto value = consume_id_or_kw(p, end);
                    tokens.push_back(make_token<token_id_or_kw>(
                        line_no, line_start, token_start, p, value.str()));
                    continue;
                }

                iter_t token_start = p;
                // we have operators
                auto value = consume_operator(p, end);
                switch (_state.pop()) {
                    case lexer_state::OPERATOR:
                        tokens.push_back(make_token<token_operator>(
                            line_no, line_start, token_start, p, value.first.str(), value.second));
                        break;
                    case lexer_state::ERROR_OPERATOR:
                        error(line_no, line_start, token_start, p,
                            "unexpected token {}", value.first.str());
                    default:
                        error(line_no, line_start, token_start, p,
                            "<internal error>: illegal state in post-done lex");
                }
            }
        }
    };
}

namespace cs {
    using lexer = cs_impl::basic_lexer<char>;
}
