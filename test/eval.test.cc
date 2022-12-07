//
// A Lox compiler
//
// Copyright © Alex Kowalenko 2022.
//

#include <iostream>
#include <sstream>
#include <string>

#include <fmt/core.h>
#include <gtest/gtest.h>

#include "compiler.hh"
#include "parser.hh"
#include "printer.hh"

struct ParseTests {
    std::string input;
    std::string output;
    std::string error;
};

auto do_eval_tests(std::vector<ParseTests> &tests) -> void;

TEST(Eval, Basic) { // NOLINT
    std::vector<ParseTests> tests = {
        {"print 0;", "0", ""},       {"print 123456;", "123456", ""},
        {"print true;", "true", ""}, {"print false;", "false", ""},
        {"print nil;", "nil", ""},   {R"(print "a";)", R"(a)", ""},
    };
    do_eval_tests(tests);
}

TEST(Eval, unary) { // NOLINT
    std::vector<ParseTests> tests = {
        {"print -2;", "-2", ""},
        {"print --2;", "2", ""},
        {"print -!-2;", "", "Operand must be a number."},
        {"print !true;", "false", ""},
    };
    do_eval_tests(tests);
}

TEST(Eval, binary) { // NOLINT
    std::vector<ParseTests> tests = {
        {"print 1+2;", "3", ""},
        {"print 2 * 3 + 4;", "10", ""},
        {"print 2 + 3 * 4;", "14", ""},
        {"print 2 + 3 * 4 / 5;", "4.4", ""},

        {"print 1 and 2;", "2", ""},
        {"print 1 and 2 or 3;", "2", ""},
        {"print 1 or 2 and 3;", "1", ""},
        {"print 1 and ! 3;", "false", ""},

        {"print 1 < 2;", "true", ""},
        {"print 1 < 2 <= 3;", "", "Operands must be numbers."},
        {"print 1 == 2 != 3;", "((1 == 2) != 3);", ""},

        {R"(print "a" + "b";)", R"((ab);)", ""},
    };
    do_eval_tests(tests);
}

TEST(Eval, grouping) { // NOLINT
    std::vector<ParseTests> tests = {
        {"print 2 * (3 + 4);", "14", ""},
        {"print (2 + 3) * 4;", "20", ""},
        {"print (2 + 3) * 4 / 5;", "4", ""},
    };
    do_eval_tests(tests);
}

TEST(Eval, var) { // NOLINT
    std::vector<ParseTests> tests = {
        {"var x = 1; print x;", "1", ""},
        {"var x; print x;", "nil", ""},
    };
    do_eval_tests(tests);
}

TEST(Eval, block) { // NOLINT
    std::vector<ParseTests> tests = {
        {"{var x = 1; print x;}", "1", ""},
        {"{var x = 1; var y = 2; print x+y;}", "3", ""},
        {"{var x = 1;} var x = 1; print x;", "1", ""},
    };
    do_eval_tests(tests);
}

inline std::string rtrim(std::string s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) { return !std::isspace(ch); })
                .base(),
            s.end());
    return s;
}

std::string nl_trim(std::string s) {
    return s.substr(0, s.find_first_of('\n'));
}

void do_eval_tests(std::vector<ParseTests> &tests) {

    std::ostringstream err;
    std::ostringstream out;
    Options            options(out, std::cin, err);
    options.silent = true;
    VM vm(options);
    gc.init(&vm);
    vm.init();
    Error errors(options.err);
    vm.set_error_manager(&errors);

    for (auto const &t : tests) {
        try {
            std::cout << t.input << " -> " << t.output;
            out.str("");
            err.str("");
            errors.reset();
            Scanner scanner(t.input);
            Parser  parser(scanner, errors);

            auto *ast = parser.parse();
            if (errors.hadError) {
                EXPECT_EQ(rtrim(err.str()), t.error);
                return;
            }

            Compiler compiler(options, errors);
            gc.set_compiler(&compiler);
            ObjFunction *function = compiler.compile(ast, &parser);
            if (errors.hadError) {
                EXPECT_EQ(rtrim(err.str()), t.error);
                return; // I
            }
            vm.run(function);
            if (errors.hadError) {
                EXPECT_EQ(nl_trim(err.str()), t.error);
                return; // I
            }
            EXPECT_EQ(rtrim(out.str()), t.output);

        } catch (std::exception &e) {
            std::cerr << "Exception: " << e.what() << std::endl;
            FAIL();
        }
    }
}