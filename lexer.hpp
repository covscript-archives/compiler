//
// Created by kiva on 2020/3/10.
//
#pragma once

#include <stack>
#include <deque>
#include <memory>
#include <mozart++/string>
#include <mozart++/iterator_range>

namespace cs_impl {
    enum class token_type {
        END_OF_FILE,
        ID_OR_KW,
        INT_LITERAL,
        FLOATING_LITERAL,
        STRING_LITERAL,
        PREPROCESSOR,
    };

    struct token {
        token_type _type = token_type::END_OF_FILE;

        explicit token(token_type type) : _type(type) {}

        virtual ~token() = default;

        token_type type() const { return _type; }
    };

    struct token_preprocessor : public token {
        std::string _value;

        explicit token_preprocessor(std::string value)
            : token(token_type::PREPROCESSOR), _value(std::move(value)) {}

        ~token_preprocessor() override = default;
    };

    struct token_id_or_kw : public token {
        std::string _value;

        explicit token_id_or_kw(std::string value)
            : token(token_type::ID_OR_KW), _value(std::move(value)) {}

        ~token_id_or_kw() override = default;
    };

    struct token_int_literal : public token {
        int64_t _value;

        explicit token_int_literal(int64_t value)
            : token(token_type::INT_LITERAL), _value(value) {}

        ~token_int_literal() override = default;
    };

    struct token_float_literal : public token {
        double _value;

        explicit token_float_literal(double value)
            : token(token_type::FLOATING_LITERAL), _value(value) {}

        ~token_float_literal() override = default;
    };

    struct token_string_literal : public token {
        std::string _value;

        explicit token_string_literal(std::string value)
            : token(token_type::STRING_LITERAL), _value(std::move(value)) {}

        ~token_string_literal() override = default;
    };

    enum class lexer_state {
        GLOBAL,
        INT_LIT,
        FLOATING_LIT,
        STRING_LIT,
        PREPROCESSOR,

        PARSING_STRING,

        ERROR_EOF,
        ERROR_ESCAPE,
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

    template <typename CharT>
    struct basic_lexer {
        using iter_t = const CharT *;
    private:
        state_manager _state;
        lexer_input<CharT> _input;

        // return true if it's id or keyword
        bool is_id_or_kw(CharT c, bool first) const {
            return std::isalpha(c)
                   || c == '$'
                   || c == '_'
                   || (!first && std::isdigit(c));
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

    public:
        void source(const std::basic_string<CharT> &str) {
            _input.source(str.c_str(), str.length());
        }

        void lex(std::deque<std::unique_ptr<token>> &tokens) {
            iter_t p = _input.begin();
            iter_t end = _input.end();

            // current line start position
            std::size_t line_no = 1;
            iter_t line_start = p;

            while (p != end) {
                // tokens only available in the beginning of a line
                if (line_start == p) {
                    if (*p == '#' || *p == '@') {
                        // comment and preprocessor tag in CovScript 3
                        auto value = consume_preprocessor(p, end);
                        switch (_state.pop()) {
                            case lexer_state::PREPROCESSOR:
                                printf("meet proprocessor: [%s]\n", value.str().c_str());
                                break;
                            default:
                                printf("should not reach here\n");
                                return;
                        }
                        continue;
                    }
                }

                // if we meet \n
                if (*p == '\n') {
                    ++line_no;
                    line_start = ++p;
                    printf("meet new line\n");
                    continue;
                }

                // skip whitespaces
                if (std::isspace(*p)) {
                    ++p;
                    continue;
                }

                // digit
                if (std::isdigit(*p)) {
                    auto result = consume_number(p, end);
                    switch (_state.pop()) {
                        case lexer_state::INT_LIT:
                            printf("meet int literal: %ld\n", result.first);
                            break;
                        case lexer_state::FLOATING_LIT:
                            printf("meet double literal: %lf\n", result.second);
                            break;
                        default:
                            printf("should not reach here\n");
                            return;
                    }
                    continue;
                }

                // string literal
                if (*p == '"') {
                    auto value = consume_string_lit(p, end);
                    switch (_state.pop()) {
                        case lexer_state::STRING_LIT:
                            printf("meet string lit: [%s]\n", value.str().c_str());
                            break;
                        case lexer_state::ERROR_EOF:
                            printf("unexpected EOF\n");
                            return;
                        case lexer_state::ERROR_ESCAPE:
                            printf("unsupported escape char\n");
                            return;
                        default:
                            printf("should not reach here\n");
                            return;
                    }
                    continue;
                }

                // id or kw
                if (is_id_or_kw(*p, true)) {
                    auto value = consume_id_or_kw(p, end);
                    printf("meet id or kw: [%s]\n", value.str().c_str());
                    continue;
                }

                printf("meet char %c\n", *p);
                ++p;
            }
        }
    };
}

namespace cs {
    using lexer = cs_impl::basic_lexer<char>;
}
