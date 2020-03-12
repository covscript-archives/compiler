//
// Created by kiva on 2020/3/10.
//
#pragma once

#include <stack>
#include <deque>
#include <memory>
#include <unordered_map>
#include <utility>
#include <mozart++/codecvt>
#include <mozart++/format>

namespace cs_impl {
    enum class token_type {
        UNDEFINED,
        ID_OR_KW,
        INT_LITERAL,
        FLOATING_LITERAL,
        STRING_LITERAL,
        CHAR_LITERAL,
        PREPROCESSOR,
        OPERATOR,
        CUSTOM_LITERAL,
    };

    enum class operator_type {
        UNDEFINED,             //
        OPERATOR_ADD,          // +
        OPERATOR_SUB,          // -
        OPERATOR_MUL,          // *
        OPERATOR_DIV,          // /
        OPERATOR_MOD,          // *
        OPERATOR_ASSIGN,       // =
        OPERATOR_ADD_ASSIGN,   // +=
        OPERATOR_SUB_ASSIGN,   // -=
        OPERATOR_MUL_ASSIGN,   // *=
        OPERATOR_DIV_ASSIGN,   // /=
        OPERATOR_MOD_ASSIGN,   // %=
        OPERATOR_AND_ASSIGN,   // &=
        OPERATOR_OR_ASSIGN,    // |=
        OPERATOR_XOR_ASSIGN,   // ^=
        OPERATOR_EQ,           // ==
        OPERATOR_NE,           // !=
        OPERATOR_GT,           // >
        OPERATOR_GE,           // >=
        OPERATOR_LT,           // <
        OPERATOR_LE,           // <=
        OPERATOR_COLON,        // :
        OPERATOR_COMMA,        // ,
        OPERATOR_QUESTION,     // ?
        OPERATOR_INC,          // ++
        OPERATOR_DEC,          // --
        OPERATOR_ARROW,        // ->
        OPERATOR_DOT,          // .
        OPERATOR_AND,          // &&
        OPERATOR_OR,           // ||
        OPERATOR_NOT,          // !
        OPERATOR_BITAND,       // &
        OPERATOR_BITOR,        // |
        OPERATOR_BITXOR,       // ^
        OPERATOR_BITNOT,       // ~
        OPERATOR_VARARG,       // ...
        OPERATOR_LPAREN,       // (
        OPERATOR_RPAREN,       // )
        OPERATOR_LBRACKET,     // [
        OPERATOR_RBRACKET,     // ]
        OPERATOR_LBRACE,       // {
        OPERATOR_RBRACE,       // }

        OPERATOR_SEMI,
    };

    ////////////////////////////////////////////////////////////////////////////////
    // tokens
    ////////////////////////////////////////////////////////////////////////////////

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
                                std::string text, std::string value,
                                operator_type type)
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

    struct token_char_literal : public token {
        char32_t _value;

        explicit token_char_literal(std::size_t line, std::size_t column,
                                    std::string text, char32_t value)
            : token(line, column, std::move(text), token_type::CHAR_LITERAL),
              _value(value) {}

        ~token_char_literal() override = default;
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

    ////////////////////////////////////////////////////////////////////////////////
    // lexer state / lexer input
    ////////////////////////////////////////////////////////////////////////////////

    enum class lexer_state {
        GLOBAL,
        INT_LIT,
        FLOATING_LIT,
        STRING_LIT,
        CHAR_LIT,
        PREPROCESSOR,
        OPERATOR,
        LITERAL_SUFFIX,

        PARSING_STRING,
        TRYING_LITERAL_SUFFIX,

        ERROR_EOF,
        ERROR_ENCLOSING,
        ERROR_ESCAPE,
        ERROR_OPERATOR,
        ERROR_EMPTY,
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

    struct lexer_input {
        using CharT = char32_t;
    private:
        std::u32string _source;
        const CharT *_begin = nullptr;
        std::size_t _length = 0;

    public:
        void source(std::u32string data) {
            std::swap(_source, data);
            this->_begin = _source.c_str();
            this->_length = _source.length();
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
        std::size_t _start_column;
        std::size_t _end_column;
        std::string _error_text;

        explicit lexer_error(std::size_t line, std::size_t start_column, std::size_t end_column,
                             std::string error_text, const std::string &message)
            : std::runtime_error(message), _line(line),
              _start_column(start_column), _end_column(end_column),
              _error_text(std::move(error_text)) {
        }

        ~lexer_error() override = default;
    };

    ////////////////////////////////////////////////////////////////////////////////
    // lexer
    ////////////////////////////////////////////////////////////////////////////////

    struct lexer {
        using CharT = char32_t;
        using iter_t = const CharT *;
    private:
        state_manager _state;
        lexer_input _input;
        std::unique_ptr<mpp::codecvt::charset> _charset;
        std::unordered_map<std::string, operator_type> _op_maps;

        template <typename T, typename ...Args>
        std::unique_ptr<token> make_token(std::size_t line, iter_t line_start,
                                          iter_t token_start, iter_t token_end,
                                          Args &&...args) {
            return std::unique_ptr<token>(
                new T{line, static_cast<std::size_t>(token_start - line_start),
                      _charset->wide2local({token_start, static_cast<std::size_t>(token_end - token_start)}),
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
                line, static_cast<std::size_t>(token_start - line_start),
                static_cast<std::size_t>(token_end - line_start),
                _charset->wide2local({token_start, static_cast<std::size_t>(token_end - token_start)}),
                message
            );
            std::terminate();
        }

        // return true if it's id or keyword
        bool is_id_or_kw(CharT c, bool first) const {
            return (c >= U'a' && c <= U'z')
                   || (c >= U'A' && c <= U'Z')
                   || c == U'$'
                   || c == U'_'
                   || _charset->is_identifier(c)
                   || (!first && is_digit_char(c));
        }

        bool is_separator_char(CharT c) const {
            return c == U' '
                   || c == U'\n'
                   || c == U'\r'
                   || c == U'\t'
                   || c == U'\f'
                   || c == U'\v'
                   || c == U';';
        }

        bool is_digit_char(CharT c) const {
            return (c >= U'0' && c <= U'9');
        }

        bool is_escape_char(CharT c) const {
            switch (c) {
                case U'r':  // \r
                case U'n':  // \n
                case U't':  // \t
                case U'b':  // \b
                case U'f':  // \f
                case U'v':  // \v
                case U'\\':
                case U'"':  // \"
                case U'\'':  // \'
                    return true;
                default:
                    return false;
            }
        }

        CharT to_escaped_char(CharT c) const {
            switch (c) {
                case U'r':
                    return U'\r';
                case U'n':
                    return U'\n';
                case U't':
                    return U'\t';
                case U'b':
                    return U'\b';
                case U'f':
                    return U'\f';
                case U'v':
                    return U'\v';
                case U'\\':
                    return U'\\';
                case U'"':
                    return U'"';
                case U'\'':
                    return U'\'';
                default:
                    // impossible here
                    return '\0';
            }
        }

        std::string consume_preprocessor(iter_t &current, iter_t end) {
            iter_t left = current;
            auto length = static_cast<std::size_t>(end - current);
            auto *s = std::char_traits<char32_t>::find(current, length, U'\n');

            current = s ? s : end;
            _state.new_state(lexer_state::PREPROCESSOR);
            return _charset->wide2local({left, static_cast<std::size_t>(current - left)});
        }

        std::pair<int64_t, double> consume_number(iter_t &current, iter_t end) {
            int64_t integer_part = *current++ - U'0';
            if (current == end) {
                _state.new_state(lexer_state::INT_LIT);
                return std::make_pair(integer_part, 0);
            }

            // lookahead for (.)
            bool found_point = false;
            iter_t lookahead = current;
            while (lookahead < end) {
                if (*current == U'.') {
                    found_point = true;
                    break;
                }
                if (is_digit_char(*lookahead)) {
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
                    if (is_digit_char(*current)) {
                        if (after_point) {
                            floating_part = floating_part * 10 + *current++ - U'0';
                            npoints *= 10;
                        } else {
                            integer_part = integer_part * 10 + *current++ - U'0';
                        }
                    } else if (*current == U'.') {
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

            if (*current == U'x' || *current == U'X') {
                // parsing hex number
                ++current;
                while (current < end && ((*current >= U'0' && *current <= U'9')
                                         || (*current >= U'a' && *current <= U'f')
                                         || (*current >= U'A' && *current <= U'F'))) {
                    integer_part = integer_part * 16
                                   + (*current & 15U)
                                   + (*current >= U'A' ? 9 : 0);
                    ++current;
                }

                return std::make_pair(integer_part, 0);

            } else if (*current == U'b' || *current == U'B') {
                // parsing binary number
                ++current;
                while (current < end && (*current == U'0' || *current == U'1')) {
                    integer_part = integer_part * 2 + *current - U'0';
                    ++current;
                }

                return std::make_pair(integer_part, 0);

            } else {
                // parsing oct number
                while (current < end && *current >= U'0' && *current <= U'7') {
                    integer_part = integer_part * 8 + *current - U'0';
                    ++current;
                }

                return std::make_pair(integer_part, 0);
            }
        }

        std::string consume_string_lit(iter_t &current, iter_t end) {
            if (*current == U'"') {
                ++current;
            }

            // string start
            iter_t left = current;
            _state.new_state(lexer_state::PARSING_STRING);

            bool escape = false;
            while (current < end && _state.current() == lexer_state::PARSING_STRING) {
                if (escape) {
                    if (is_escape_char(*current)) {
                        escape = false;
                        ++current;
                    } else {
                        // invalid escape char
                        _state.replace(lexer_state::ERROR_ESCAPE);
                    }
                } else {
                    switch (*current) {
                        case U'\\':
                            escape = true;
                            ++current;
                            break;
                        case U'"':
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
                    return _charset->wide2local({left, static_cast<std::size_t>(current - left - 1)});
                case lexer_state::PARSING_STRING:
                    // unexpected EOF when parsing string
                    _state.replace(lexer_state::ERROR_EOF);
                    // fall-through
                default:
                    // error happened, reason stored in state
                    return std::string{};
            }
        }

        char32_t consume_char_lit(iter_t &current, iter_t end) {
            if (*current == U'\'') {
                ++current;
            }

            if (current == end) {
                _state.new_state(lexer_state::ERROR_EMPTY);
                return 0;
            }

            bool escape = false;
            bool found = false;

            while (current < end) {
                if (escape) {
                    if (is_escape_char(*current)) {
                        found = true;
                        break;
                    } else {
                        _state.new_state(lexer_state::ERROR_ESCAPE);
                        return 0;
                    }
                } else {
                    if (*current == U'\\') {
                        escape = true;
                        ++current;
                    } else if (*current == U'\'') {
                        break;
                    } else {
                        found = true;
                        break;
                    }
                }
            }

            if (!found) {
                // no char was found
                _state.new_state(current == end ? lexer_state::ERROR_EOF : lexer_state::ERROR_EMPTY);
                return 0;
            }

            CharT lit = *current++;
            if (current == end) {
                // there's one more `'`
                _state.new_state(lexer_state::ERROR_EOF);
                return 0;
            }

            if (*current != '\'') {
                // there's one more `'`
                _state.new_state(lexer_state::ERROR_ENCLOSING);
                return 0;
            }

            // consume the closing `'`
            ++current;
            _state.new_state(lexer_state::CHAR_LIT);
            return escape ? to_escaped_char(lit) : lit;
        }

        std::string consume_id_or_kw(iter_t &current, iter_t end) {
            // start part
            iter_t left = current++;
            while (current < end && is_id_or_kw(*current, false)) {
                ++current;
            }
            return _charset->wide2local({left, static_cast<std::size_t>(current - left)});
        }

        std::pair<std::string, operator_type> consume_operator(iter_t &current, iter_t end) {
            iter_t left = current;

            // be greedy, be lookahead
            while (current < end
                   && !is_separator_char(*current)
                   && !is_id_or_kw(*current, false)) {
                ++current;
            }

            iter_t most = current;
            while (current != left) {
                std::string op = _charset->wide2local({left, static_cast<std::size_t>(current - left)});
                auto iter = _op_maps.find(op);
                if (iter != _op_maps.end()) {
                    _state.new_state(lexer_state::OPERATOR);
                    return std::make_pair(op, iter->second);
                }
                // lookahead failed, try previous one
                --current;
            }

            _state.new_state(lexer_state::ERROR_OPERATOR);
            return std::make_pair(
                _charset->wide2local({left, static_cast<std::size_t>(most - left)}),
                operator_type::UNDEFINED);
        }

        std::string try_consume_literal_suffix(std::deque<std::unique_ptr<token>> &tokens,
                                               std::size_t line_no, iter_t line_start,
                                               iter_t &current, iter_t end) {
            // got literal suffix
            if (*current != U'_') {
                return std::string{};
            }

            auto value = consume_id_or_kw(current, end);
            _state.new_state(lexer_state::LITERAL_SUFFIX);
            return value;
        }

    public:
        explicit lexer(std::unique_ptr<mpp::codecvt::charset> charset)
            : _charset(std::move(charset)) {
        }

        void source(const std::string &str) {
            _input.source(_charset->local2wide(str));
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

            while (p < end) {
                /////////////////////////////////////////////////////////////////
                // special position
                /////////////////////////////////////////////////////////////////
                // tokens only available in the beginning of a line
                if (line_start == p) {
                    if (*p == U'#' || *p == U'@') {
                        // comment and preprocessor tag in CovScript 3
                        iter_t token_start = p;
                        auto value = consume_preprocessor(p, end);
                        switch (_state.pop()) {
                            case lexer_state::PREPROCESSOR:
                                tokens.push_back(make_token<token_preprocessor>(line_no, line_start,
                                    token_start, p, value));
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
                            case token_type::INT_LITERAL:
                            case token_type::FLOATING_LITERAL:
                            case token_type::STRING_LITERAL:
                            case token_type::CHAR_LITERAL:
                                break;
                            default:
                                error(line_no, line_start, token_start, p,
                                    "unsupported literal suffix {} after non-literal", value);
                        }

                        auto literal = std::move(tokens.back());
                        tokens.pop_back();
                        tokens.push_back(make_token<token_custom_literal>(
                            line_no, line_start, token_start, p,
                            std::move(literal), value));
                    }
                    continue;
                }

                /////////////////////////////////////////////////////////////////
                // global state
                /////////////////////////////////////////////////////////////////

                // if we meet \n
                if (*p == U'\n') {
                    ++line_no;
                    line_start = ++p;
                    continue;
                }

                // skip separators
                if (is_separator_char(*p)) {
                    ++p;
                    continue;
                }

                // digit
                if (is_digit_char(*p)) {
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
                if (*p == U'"') {
                    iter_t token_start = p;
                    auto value = consume_string_lit(p, end);
                    switch (_state.pop()) {
                        case lexer_state::STRING_LIT:
                            tokens.push_back(make_token<token_string_literal>(
                                line_no, line_start, token_start, p, value));
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

                // char literal
                if (*p == U'\'') {
                    iter_t token_start = p;
                    auto ch = consume_char_lit(p, end);
                    switch (_state.pop()) {
                        case lexer_state::CHAR_LIT:
                            tokens.push_back(make_token<token_char_literal>(
                                line_no, line_start, token_start, p, ch));
                            // try parse literal suffix
                            _state.new_state(lexer_state::TRYING_LITERAL_SUFFIX);
                            break;
                        case lexer_state::ERROR_EOF:
                            printf("unexpected EOF\n");
                            error(line_no, line_start, token_start, p,
                                "unexpected EOF");
                        case lexer_state::ERROR_ESCAPE:
                            error(line_no, line_start, token_start, p,
                                "unsupported escape char: `\\{}`", *p);
                        case lexer_state::ERROR_EMPTY:
                            error(line_no, line_start, token_start, p,
                                "empty char is not allowed");
                        case lexer_state::ERROR_ENCLOSING:
                            error(line_no, line_start, token_start, p,
                                "unclosed char literal, expected `'`");
                        default:
                            error(line_no, line_start, token_start, p,
                                "<internal error>: illegal state in char literal");
                    }
                    continue;
                }

                // id or kw
                if (is_id_or_kw(*p, true)) {
                    iter_t token_start = p;
                    auto value = consume_id_or_kw(p, end);
                    tokens.push_back(make_token<token_id_or_kw>(
                        line_no, line_start, token_start, p, value));
                    continue;
                }

                iter_t token_start = p;
                // we have operators
                auto value = consume_operator(p, end);
                switch (_state.pop()) {
                    case lexer_state::OPERATOR:
                        tokens.push_back(make_token<token_operator>(
                            line_no, line_start, token_start, p, value.first, value.second));
                        break;
                    case lexer_state::ERROR_OPERATOR:
                        error(line_no, line_start, token_start, p,
                            "unexpected token '{}'", value.first);
                    default:
                        error(line_no, line_start, token_start, p,
                            "<internal error>: illegal state in post-done lex");
                }
            }
        }
    };
}

namespace cs {
    using cs_impl::lexer;
}
