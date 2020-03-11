#include <iostream>
#include "lexer.hpp"

int main() {
    using cs_impl::token_type;

    cs::lexer lexer;
    std::string code = "#!/usr/bin/env cs4\n"
                       "test0 0 1 2 3 4 5 6 7\n"
                       "fuck0 0.1 0.4 11.21 112.33321 1234567\n"
                       "end122\n"
                       "n0x123 0b123 0123\n"
                       "error0s 0x7FFFz\n"
                       "oct 0123 0755\n"
                       "str \"fuck you\"\n"
                       "str \"\"\n"
                       "str \"hello\\n\\r\\t\\e\\x\\0world\"\n"
                       "test-multi-line \"this is a multi-\n  line\n  string\"\n"
                       "";

    lexer.source(code);

    std::deque<std::unique_ptr<cs_impl::token>> d;
    lexer.add_operators({
        {"+", token_type::OPERATOR_ADD},
        {"-", token_type::OPERATOR_SUB},
        {"*", token_type::OPERATOR_MUL},
        {"/", token_type::OPERATOR_DIV},
        {"%", token_type::OPERATOR_MOD},
        {"=", token_type::OPERATOR_ASSIGN},
        {"+=", token_type::OPERATOR_ADD_ASSIGN},
        {"-=", token_type::OPERATOR_SUB_ASSIGN},
        {"*=", token_type::OPERATOR_MUL_ASSIGN},
        {"/=", token_type::OPERATOR_DIV_ASSIGN},
        {"%=", token_type::OPERATOR_MOD_ASSIGN},
        {"&=", token_type::OPERATOR_AND_ASSIGN},
        {"|=", token_type::OPERATOR_OR_ASSIGN},
        {"^=", token_type::OPERATOR_XOR_ASSIGN},
        {"==", token_type::OPERATOR_EQ},
        {"!=", token_type::OPERATOR_NE},
        {">", token_type::OPERATOR_GT},
        {">=", token_type::OPERATOR_GE},
        {"<", token_type::OPERATOR_LT},
        {"<=", token_type::OPERATOR_LE},
        {":", token_type::OPERATOR_COLON},
        {",", token_type::OPERATOR_COMMA},
        {"?", token_type::OPERATOR_QUESTION},
        {"++", token_type::OPERATOR_INC},
        {"--", token_type::OPERATOR_DEC},
        {"->", token_type::OPERATOR_ARROW},
        {"&&", token_type::OPERATOR_AND},
        {"||", token_type::OPERATOR_OR},
        {"!", token_type::OPERATOR_NOT},
        {"&", token_type::OPERATOR_BITAND},
        {"|", token_type::OPERATOR_BITOR},
        {"^", token_type::OPERATOR_BITXOR},
        {"~", token_type::OPERATOR_BITNOT},
        {"...", token_type::OPERATOR_VARARG},
        {"(", token_type::OPERATOR_LPAREN},
        {")", token_type::OPERATOR_RPAREN},
        {"[", token_type::OPERATOR_LBRACKET},
        {"]", token_type::OPERATOR_RBRACKET},
        {"{", token_type::OPERATOR_LBRACE},
        {"}", token_type::OPERATOR_RBRACE},
    });
    lexer.lex(d);
}
