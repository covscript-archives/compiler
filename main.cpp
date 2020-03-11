#include <iostream>
#include "lexer.hpp"

int main() {
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
    lexer.lex(d);
}
