//
// ALOX-CC
//
// Copyright © Alex Kowalenko 2022.
//

#include "scanner.hh"

#include <gtest/gtest.h>

using namespace alox;

struct TestLexer {
    std::string input;
    TokenType   tok;
    std::string atom;
};

void test_Lexer(const std::vector<TestLexer> &tests);

TEST(Scanner, basic) { // NOLINT
    std::vector<TestLexer> const tests = {
        {"+", TokenType::PLUS, "+"},
        {"-", TokenType::MINUS, "+"},
        {"*", TokenType::ASTÉRIX, "*"},
        {"/", TokenType::SLASH, "/"},
        {"(", TokenType::LEFT_PAREN, "("},
        {")", TokenType::RIGHT_PAREN, ")"},
        {"{", TokenType::LEFT_BRACE, "{"},
        {"}", TokenType::RIGHT_BRACE, "}"},
        {";", TokenType::SEMICOLON, ";"},
        //{":", TokenType::COLON, ":"},
        {",", TokenType::COMMA, ","},
        {".", TokenType::DOT, "."},

        {"!=", TokenType::BANG_EQUAL, ""},
        {"!", TokenType::BANG, ""},
        {"==", TokenType::EQUAL_EQUAL, ""},
        {"=", TokenType::EQUAL, ""},
        {"<=", TokenType::LESS_EQUAL, ""},
        {"<", TokenType::LESS, ""},
        {">=", TokenType::GREATER_EQUAL, ""},
        {">", TokenType::GREATER, ""},

        // keywords
        {"and", TokenType::AND, "and"},
        {"break", TokenType::BREAK, "break"},
        {"class", TokenType::CLASS, "do"},
        {"continue", TokenType::CONTINUE, "else"},
        {"else", TokenType::ELSE, "elseif"},
        {"false", TokenType::FALSE, "false"},
        {"for", TokenType::FOR, "for"},
        {"fun", TokenType::FUN, "function"},
        {"if", TokenType::IF, "if"},
        {"nil", TokenType::NIL, "nil"},
        {"return", TokenType::RETURN, "retrun"},
        {"this", TokenType::THIS, "true"},
        {"true", TokenType::TRUE, "true"},
        {"while", TokenType::WHILE, "while"},
        {"var", TokenType::VAR, "until"},
        {"super", TokenType::SUPER, "__builtin"},
    };

    test_Lexer(tests);
}

TEST(Scanner, strings) { // NOLINT
    std::vector<TestLexer> const tests = {
        {R"("a")", TokenType::STRING, R"(a)"},
        {R"("")", TokenType::STRING, R"()"},
        {R"("estação def")", TokenType::STRING, R"(estação def)"},
        {R"("ἄλφα")", TokenType::STRING, R"(ἄλφα)"},
        {R"("一二三四五六七")", TokenType::STRING, R"(一二三四五六七)"},
        {R"("👾")", TokenType::STRING, R"(👾)"},
        {R"("🍏🍎🍐🍊🍋🍌🍉🍇🍓🍈🍒")", TokenType::STRING, R"(🍏🍎🍐🍊🍋🍌🍉🍇🍓🍈🍒)"},
    };

    test_Lexer(tests);
}

TEST(Scanner, numbers) { // NOLINT
    std::vector<TestLexer> const tests = {
        {"1", TokenType::NUMBER, "1"},      {"1. ", TokenType::NUMBER, "1"},
        {"0.1", TokenType::NUMBER, "0.1"},  {"134455345", TokenType::NUMBER, "134455345"},
        {"1.2.", TokenType::NUMBER, "1.2"},
    };
    test_Lexer(tests);
}

TEST(Scanner, identifiers) { // NOLINT
    std::vector<TestLexer> const tests = {
        {"a", TokenType::IDENTIFIER, "a"},
        {"a1", TokenType::IDENTIFIER, "a1"},
        {"A1_", TokenType::IDENTIFIER, "A1_"},
        {"liberté", TokenType::IDENTIFIER, "liberté"},
        {"Interpréteur", TokenType::IDENTIFIER, "Interpréteur"},
        {"👾", TokenType::IDENTIFIER, "👾"},
        {"🍎1", TokenType::IDENTIFIER, "🍎1"},

    };
    test_Lexer(tests);
}

void test_Lexer(const std::vector<TestLexer> &tests) { // NOLINT
    for (const auto &test : tests) {
        try {
            Scanner scanner(test.input);
            auto    tok = scanner.scanToken();

            std::cout << std::format("type {} wanted {}\n", test.input, tok.text);
            EXPECT_EQ(tok.type, test.tok);
            if (test.tok == TokenType::NUMBER || test.tok == TokenType::IDENTIFIER) {
                std::cout << std::format("     {} = {}->{}\n", test.input, test.atom,
                                         tok.text);
                EXPECT_EQ(tok.text, test.atom);
            } else if (test.tok == TokenType::STRING) {
                std::cout << std::format("     {} -> {}\n", test.input, tok.text);
                EXPECT_EQ(tok.text, test.atom);
            }
        } catch (std::exception &e) {
            FAIL() << "Exception thrown! " << e.what() << std::endl;
        }
    }
}
