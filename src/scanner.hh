//
// ALOX-CC
//

#pragma once

#include <string_view>

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
    TokenType        type;
    std::string_view text;
    int              line;
};

class Scanner {
  public:
    explicit Scanner(const char *source);
    Token scanToken();

  private:
    [[nodiscard]] constexpr bool isAtEnd() const { return *current == '\0'; };
    constexpr char               advance() {
        current++;
        return current[-1];
    }
    [[nodiscard]] constexpr char peek() const { return *current; }
    [[nodiscard]] constexpr char peekNext() const {
        return isAtEnd() ? '\0' : current[1];
    };

    bool      match(char expected);
    Token     makeToken(TokenType type);
    Token     errorToken(const char *message) const;
    void      skipWhitespace();
    TokenType checkKeyword(int start, int length, const char *rest, TokenType type);
    TokenType identifierType();
    Token     identifier();
    Token     number();
    Token     string();

    const char *start;
    const char *current;
    int         line{1};
};
