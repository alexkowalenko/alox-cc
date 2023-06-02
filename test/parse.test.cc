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

using namespace alox;

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
        {"* 3", "", "[line 1] Error at '*': Expect expression."},
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
        {"var = ;", "", "[line 1] Error at '=': Expect variable name."},
    };
    do_parse_tests(tests);
}

TEST(Parser, assign) { // NOLINT
    std::vector<ParseTests> tests = {
        {"x = 1;", "x = 1;", ""},
        {"jones = 2 + x;", "jones = (2 + x);", ""},

        // Error
        {"x = ;", "", "[line 1] Error at ';': Expect expression."},
        {"= 2 ;", "", "[line 1] Error at '=': Expect expression."},
    };
    do_parse_tests(tests);
}

TEST(Parser, block) { // NOLINT
    std::vector<ParseTests> tests = {
        {"{var x = 1;}", "{ var x = 1; }", ""},
        {"{var x = 1; var y = 2;}", "{ var x = 1; var y = 2; }", ""},
        {"{}", "{ }", ""},

        // Error
        {"{ print 1;", "", "[line 1] Error at end: Expect '}' after block."},
        {"print 1; }", "var x;", "[line 1] Error at '}': Expect expression."},
    };
    do_parse_tests(tests);
}

TEST(Parser, if) { // NOLINT
    std::vector<ParseTests> tests = {
        {"if (true) print 1;", "if (true) print 1;", ""},
        {"if (true) print 1; else print 2;", "if (true) print 1; else print 2;", ""},
        {"if (true) {} else {print 1;}", "if (true) { } else { print 1; }", ""},
        {"if (true) if (true) {} else {}", "if (true) if (true) { } else { }", ""},

        // Error
        {"if true) print 1;", "", "[line 1] Error at 'true': Expect '(' after 'if'."},
    };
    do_parse_tests(tests);
}

TEST(Parser, while) { // NOLINT
    std::vector<ParseTests> tests = {
        {"while (true) print 1;", "while (true) print 1;", ""},
        {"while (true) {print 1;}", "while (true) { print 1; }", ""},
        {"while (true) { print 1; print 2;}", "while (true) { print 1; print 2; }", ""},
        {"while (a == b) {} ", "while ((a == b)) { }", ""},

        // Error
        {"while true) print 1;", "",
         "[line 1] Error at 'true': Expect '(' after 'while'."},
    };
    do_parse_tests(tests);
}

TEST(Parser, for) { // NOLINT
    std::vector<ParseTests> tests = {
        {"for (;;) true;", "for (;;) true;", ""},
        {"for (true;;) {}", "for (true;;) { }", ""},
        {"for (var x = 1;;) {}", "for (var x = 1;;) { }", ""},
        {"for (; true ;) {}", "for (;true;) { }", ""},
        {"for (; ;true) {}", "for (;;true) { }", ""},
        {"for (true; true ; true) { true;}", "for (true;true;true) { true; }", ""},
        {"for (var x= 1; x < 10 ; x = x + 1) { true;}",
         "for (var x = 1;(x < 10);x = (x + 1)) { true; }", ""},

        // Error
        {"for true; true ; true) { true;}", "",
         "[line 1] Error at 'true': Expect '(' after 'for'."},
        {"for (true; true ; true { true;}", "",
         "[line 1] Error at '{': Expect ')' after for clauses."},
    };
    do_parse_tests(tests);
}

TEST(Parser, break) { // NOLINT
    std::vector<ParseTests> tests = {
        {"while(true) {break;}", "while (true) { break; }", ""},
        {"for(;;) {continue;}", "for (;;) { continue; }", ""},
    };
    do_parse_tests(tests);
}

TEST(Parser, return) { // NOLINT
    std::vector<ParseTests> tests = {
        {"return;", "return;", ""},
        {"return 2;", "return 2;", ""},
        {"return 2 * 8;", "return (2 * 8);", ""},

        // Errors
        {"return", "", "[line 1] Error at end: Expect expression."},
        {"return 2 2;", "", "[line 1] Error at '2': Expect ';' after value."},
    };
    do_parse_tests(tests);
}

TEST(Parser, function) { // NOLINT
    std::vector<ParseTests> tests = {
        {"fun f() {}", "fun f() { }", ""},
        {"fun f(a) {}", "fun f(a) { }", ""},
        {"fun f(a,b) {}", "fun f(a, b) { }", ""},

        {"fun f() {print 1;}", "fun f() { print 1; }", ""},
        {"fun f() {print 1;print 2;}", "fun f() { print 1; print 2; }", ""},
        {"fun f(a) {print a;}", "fun f(a) { print a; }", ""},
        {"fun f(a,b) {print a;print b;}", "fun f(a, b) { print a; print b; }", ""},

        // Errors
        {"fun f ) {}", "", "[line 1] Error at ')': Expect '(' after function name."},
        {"fun f( {}", "", "[line 1] Error at '{': Expect parameter name."},
        {"fun f(a, ) {}", "", "[line 1] Error at ')': Expect parameter name."},
        {"fun f() }", "", "[line 1] Error at '}': Expect '{' before function body."},
    };
    do_parse_tests(tests);
}

TEST(Parser, class) { // NOLINT
    std::vector<ParseTests> tests = {
        {"class A {}", "class A { }", ""},
        {"class A { f() {} }", "class A { f() { } }", ""},
        {"class A < B {}", "class A < B { }", ""},
        {"class A < B { f() {} }", "class A < B { f() { } }", ""},

        // Errors
        {"class {}", "", "[line 1] Error at '{': Expect class name."},
        {"class A < { f() {} }", "", "[line 1] Error at '{': Expect superclass name."},
        {"class A < B {x}", "", "[line 1] Error at '}': Expect '(' after method name."},
        {"class A < A {}", "",
         "[line 1] Error at 'A': A class can't inherit from itself."},
    };
    do_parse_tests(tests);
}

TEST(Parser, dot) { // NOLINT
    std::vector<ParseTests> tests = {
        {"x.s;", "x.s;", ""},
        {"x.f(1,2);", "x.f(1, 2);", ""},
        {"x.s = 2;", "x.s = 2;", ""},

        // Errors
        {"x.", "", "[line 1] Error at end: Expect property name after '.'."},
        {"x.f(", "", "[line 1] Error at end: Expect expression."},
        {"x.s  2", "", "[line 1] Error at '2': Expect ';' after expression."},
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
            Scanner            scanner(t.input);
            std::ostringstream err;
            ErrorManager       errors(err);
            Parser             parser(scanner, errors);

            auto ast = parser.parse();
            if (errors.hadError) {
                EXPECT_EQ(rtrim(err.str()), t.error);
                continue; // INTERPRET_PARSE_ERROR;
            }
            std::stringstream os;
            AST_Printer       printer(os, ' ', 0);
            printer.print(ast);
            EXPECT_EQ(rtrim(os.str()), t.output);

        } catch (std::exception &e) {
            std::cerr << "Exception: " << e.what() << std::endl;
            FAIL();
        }
    }
}