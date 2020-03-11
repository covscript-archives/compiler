#include <iostream>
#include "lexer.hpp"

int main() {
    using namespace cs_impl;

    cs::lexer lexer;
    std::string code = "#!/usr/bin/env cs4\n"
                       "var text = \"hello world\"\n"
                       "system.out.println(text)\n"
                       "f(text) g(text)\n"
                       "h(text);p(text)\n"
                       "a(text);\n"
                       "b(text)\n"
                       "var hi = \"hello\\n\" +"
                       "    \"world\\n\" +"
                       "    \", I love\""
                       "var me = 12304"
                       "var e = \"\"";

    lexer.source(code);

    std::deque<std::unique_ptr<token>> d;
    lexer.add_operators({
        {"+",   operator_type::OPERATOR_ADD},
        {"-",   operator_type::OPERATOR_SUB},
        {"*",   operator_type::OPERATOR_MUL},
        {"/",   operator_type::OPERATOR_DIV},
        {"%",   operator_type::OPERATOR_MOD},
        {"=",   operator_type::OPERATOR_ASSIGN},
        {"+=",  operator_type::OPERATOR_ADD_ASSIGN},
        {"-=",  operator_type::OPERATOR_SUB_ASSIGN},
        {"*=",  operator_type::OPERATOR_MUL_ASSIGN},
        {"/=",  operator_type::OPERATOR_DIV_ASSIGN},
        {"%=",  operator_type::OPERATOR_MOD_ASSIGN},
        {"&=",  operator_type::OPERATOR_AND_ASSIGN},
        {"|=",  operator_type::OPERATOR_OR_ASSIGN},
        {"^=",  operator_type::OPERATOR_XOR_ASSIGN},
        {"==",  operator_type::OPERATOR_EQ},
        {"!=",  operator_type::OPERATOR_NE},
        {">",   operator_type::OPERATOR_GT},
        {">=",  operator_type::OPERATOR_GE},
        {"<",   operator_type::OPERATOR_LT},
        {"<=",  operator_type::OPERATOR_LE},
        {":",   operator_type::OPERATOR_COLON},
        {",",   operator_type::OPERATOR_COMMA},
        {"?",   operator_type::OPERATOR_QUESTION},
        {"++",  operator_type::OPERATOR_INC},
        {"--",  operator_type::OPERATOR_DEC},
        {"->",  operator_type::OPERATOR_ARROW},
        {".",   operator_type::OPERATOR_DOT},
        {"&&",  operator_type::OPERATOR_AND},
        {"||",  operator_type::OPERATOR_OR},
        {"!",   operator_type::OPERATOR_NOT},
        {"&",   operator_type::OPERATOR_BITAND},
        {"|",   operator_type::OPERATOR_BITOR},
        {"^",   operator_type::OPERATOR_BITXOR},
        {"~",   operator_type::OPERATOR_BITNOT},
        {"...", operator_type::OPERATOR_VARARG},
        {"(",   operator_type::OPERATOR_LPAREN},
        {")",   operator_type::OPERATOR_RPAREN},
        {"[",   operator_type::OPERATOR_LBRACKET},
        {"]",   operator_type::OPERATOR_RBRACKET},
        {"{",   operator_type::OPERATOR_LBRACE},
        {"}",   operator_type::OPERATOR_RBRACE},
        {";",   operator_type::OPERATOR_SEMI},
    });
    lexer.lex(d);

    for (const auto &token : d) {
        switch (token->_type) {
            case token_type::ID_OR_KW:
                printf(":: ID or KW: [%s]\n", static_cast<token_id_or_kw *>(token.get())->_value.c_str());
                break;
            case token_type::INT_LITERAL:
                printf(":: int literal: [%ld]\n", static_cast<token_int_literal *>(token.get())->_value);
                break;
            case token_type::FLOATING_LITERAL:
                printf(":: float literal: [%lf]\n", static_cast<token_float_literal *>(token.get())->_value);
                break;
            case token_type::STRING_LITERAL:
                printf(":: string literal: [%s]\n", static_cast<token_string_literal *>(token.get())->_value.c_str());
                break;
            case token_type::OPERATOR:
                printf(":: operator: [%s]\n", static_cast<token_operator *>(token.get())->_value.c_str());
                break;
            case token_type::PREPROCESSOR:
            case token_type::UNDEFINED:
                break;
        }
    }
}
