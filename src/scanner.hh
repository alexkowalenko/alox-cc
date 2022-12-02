//
// ALOX-CC
//

#pragma once

#include <string>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-conversion"
#include <utf8.h>
#pragma clang diagnostic pop

#include "common.hh"

#undef FALSE
#undef TRUE

enum class TokenType {
    // Single-character tokens.
    LEFT_PAREN,
    RIGHT_PAREN,
    LEFT_BRACE,
    RIGHT_BRACE,
    COMMA,
    DOT,
    MINUS,
    PLUS,
    SEMICOLON,
    SLASH,
    ASTÉRIX,
    // One or two character tokens.
    BANG,
    BANG_EQUAL,
    EQUAL,
    EQUAL_EQUAL,
    GREATER,
    GREATER_EQUAL,
    LESS,
    LESS_EQUAL,
    // Literals.
    IDENTIFIER,
    STRING,
    NUMBER,
    // Keywords.
    AND,
    BREAK,
    CLASS,
    CONTINUE,
    ELSE,
    FALSE,
    FOR,
    FUN,
    IF,
    NIL,
    OR,
    PRINT,
    RETURN,
    SUPER,
    THIS,
    TRUE,
    VAR,
    WHILE,

    ERROR,
    EOFS
};

struct Token {
    TokenType   type;
    std::string text;
    int         line;
};

class Scanner {
  public:
    explicit Scanner(const std::string &source);
    Token scanToken();

  private:
    Token error_token(const char *message) const;

    Char peek() {
        return (current != end(source)) ? utf8::peek_next(current, end(source)) : 0;
    };
    Char prior() { return utf8::prior(current, end(source)); };
    Char scan() {
        return (current != end(source)) ? utf8::next(current, end(source)) : 0;
    };

    Char get_char();

    Token get_string();
    Token get_number(Char c);
    Token get_identifier(Char c);

    const std::string          &source;
    std::string::const_iterator current;

    int line{1};
};
