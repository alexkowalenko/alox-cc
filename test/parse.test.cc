//
// A Lox compiler
//
// Copyright © Alex Kowalenko 2022.
//

#include <sstream>
#include <string>

#include <fmt/core.h>
#include <gtest/gtest.h>

#include "parser.hh"
#include "printer.hh"

struct ParseTests {
    std::string input;
    std::string output;
    std::string error;
};

auto do_parse_tests(std::vector<ParseTests> &tests) -> void;

TEST(Parser, Basic) { // NOLINT
    std::vector<ParseTests> tests = {
        {"0;", "0;", ""},         {"123456;", "123456;", ""}, {"true;", "true;", ""},
        {"false;", "false;", ""}, {"nil;", "nil;", ""},       {R"("a";)", R"("a";)", ""},
        {R"("";)", R"("";)", ""},

    };
    do_parse_tests(tests);
}

TEST(Parser, unary) { // NOLINT
    std::vector<ParseTests> tests = {
        {"-2;", "-2;", ""},
        {"--2;", "--2;", ""},
        {"-!-2;", "-!-2;", ""},
        {"!true;", "!true;", ""},

        // errors
        {"+true", "", "[line 1] Error at '+': Expect expression."},
    };
    do_parse_tests(tests);
}

TEST(Parser, binary) { // NOLINT
    std::vector<ParseTests> tests = {
        {"1+2;", "(1 + 2);", ""},
        {"2 * 3 + 4;", "((2 * 3) + 4);", ""},
        {"2 + 3 * 4;", "(2 + (3 * 4));", ""},
        {"2 + 3 * 4 / 5;", "(2 + ((3 * 4) / 5));", ""},

        {"1 and 2;", "(1 and 2);", ""},
        {"1 and 2 or 3;", "((1 and 2) or 3);", ""},
        {"1 or 2 and 3;", "(1 or (2 and 3));", ""},
        {"1 and ! 3;", "(1 and !3);", ""},

        {"1 < 2;", "(1 < 2);", ""},
        {"1 < 2 <= 3;", "((1 < 2) <= 3);", ""},
        {"1 > 2 >= 3;", "((1 > 2) >= 3);", ""},
        {"1 == 2 != 3;", "((1 == 2) != 3);", ""},

        {R"("a" + "b";)", R"(("a" + "b");)", ""},

        // Error
        {"2 +", "", "[line 1] Error at end: Expect expression."},
        {"* 3", "", "unexpected *"},
    };
    do_parse_tests(tests);
}

TEST(Parser, grouping) { // NOLINT
    std::vector<ParseTests> tests = {
        {"2 * (3 + 4);", "(2 * (3 + 4));", ""},
        {"2 + 3 * 4;", "(2 + (3 * 4));", ""},
        {"(2 + 3) * 4 / 5;", "(((2 + 3) * 4) / 5);", ""},

        // Error
        {"(3 + 4;", "", "[line 1] Error at ';': Expect ')' after expression."},
    };
    do_parse_tests(tests);
}

TEST(Parser, print) { // NOLINT
    std::vector<ParseTests> tests = {
        {"print 0;", "print 0;", ""},
        {"print true;", "print true;", ""},
        {"print false;", "print false;", ""},
        {"print nil;", "print nil;", ""},
        {R"(print "a";)", R"(print "a";)", ""},
        {R"(print "";)", R"(print "";)", ""},
        {"print 123456;", "print 123456;", ""},

        // // Error
        {"print ;", "", "[line 1] Error at ';': Expect expression."},
    };
    do_parse_tests(tests);
}

TEST(Parser, var) { // NOLINT
    std::vector<ParseTests> tests = {
        {"var x = 1;", "var x = 1;", ""},
        {"var x;", "var x;", ""},

        // Error
        {"var ;", "", "[line 1] Error at ';': Expect variable name."},
        {"var = ;", "", "unexpected = expecting <ident>"},
    };
    do_parse_tests(tests);
}

std::string rtrim(std::string s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) { return !std::isspace(ch); })
                .base(),
            s.end());
    return s;
}

void do_parse_tests(std::vector<ParseTests> &tests) {

    for (auto const &t : tests) {
        try {
            std::cout << t.input << std::endl;
            auto              scanner = Scanner(t.input);
            std::stringstream err;
            auto              parser = Parser(scanner, err);

            auto ast = parser.parse();
            if (parser.hadError) {
                EXPECT_EQ(rtrim(err.str()), t.error);
                return; // INTERPRET_PARSE_ERROR;
            }
            std::stringstream os;
            AST_Printer       printer(os, ' ');
            printer.print(ast);
            EXPECT_EQ(rtrim(os.str()), t.output);

        } catch (std::exception &e) {
            std::cerr << "Exception: " << e.what() << std::endl;
            FAIL();
        }
    }
}