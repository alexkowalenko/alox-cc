//
// ALOX-CC
//

#include <cstdio>
#include <cstring>

#include "common.hh"
#include "scanner.hh"

Scanner::Scanner(const char *source) : start(source), current(source) {}

static constexpr bool isAlpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static constexpr bool isDigit(char c) {
    return c >= '0' && c <= '9';
}

bool Scanner::match(char expected) {
    if (isAtEnd()) {
        return false;
    }
    if (*current != expected) {
        return false;
    }
    current++;
    return true;
}

Token Scanner::makeToken(TokenType type) {
    Token token{};
    token.type = type;
    token.text = {start, static_cast<size_t>((int)(current - start))};
    token.line = line;
    return token;
}

Token Scanner::errorToken(const char *message) {
    Token token{};
    token.type = TokenType::ERROR;
    token.text = {message, strlen(message)};
    token.line = line;
    return token;
}

void Scanner::skipWhitespace() {
    for (;;) {
        char c = peek();
        switch (c) {
        case ' ':
        case '\r':
        case '\t':
            advance();
            break;
        case '\n':
            line++;
            advance();
            break;
        case '/':
            if (peekNext() == '/') {
                // A comment goes until the end of the line.
                while (peek() != '\n' && !isAtEnd()) {
                    advance();
                }
            } else {
                return;
            }
            break;
        default:
            return;
        }
    }
}

TokenType Scanner::checkKeyword(int st, int length, const char *rest, TokenType type) {
    if (current - start == st + length && memcmp(start + st, rest, length) == 0) {
        return type;
    }
    return TokenType::IDENTIFIER;
}

TokenType Scanner::identifierType() {
    switch (start[0]) {
    case 'a':
        return checkKeyword(1, 2, "nd", TokenType::AND);
    case 'b':
        return checkKeyword(1, 4, "reak", TokenType::BREAK);
    case 'c':
        if (current - start > 1) {
            switch (start[1]) {
            case 'l':
                return checkKeyword(2, 3, "ass", TokenType::CLASS);
            case 'o':
                return checkKeyword(2, 6, "ntinue", TokenType::CONTINUE);
            }
        }
        break;
    case 'e':
        return checkKeyword(1, 3, "lse", TokenType::ELSE);
    case 'f':
        if (current - start > 1) {
            switch (start[1]) {
            case 'a':
                return checkKeyword(2, 3, "lse", TokenType::FALSE);
            case 'o':
                return checkKeyword(2, 1, "r", TokenType::FOR);
            case 'u':
                return checkKeyword(2, 1, "n", TokenType::FUN);
            }
        }
        break;
    case 'i':
        return checkKeyword(1, 1, "f", TokenType::IF);
    case 'n':
        return checkKeyword(1, 2, "il", TokenType::NIL);
    case 'o':
        return checkKeyword(1, 1, "r", TokenType::OR);
    case 'p':
        return checkKeyword(1, 4, "rint", TokenType::PRINT);
    case 'r':
        return checkKeyword(1, 5, "eturn", TokenType::RETURN);
    case 's':
        return checkKeyword(1, 4, "uper", TokenType::SUPER);
    case 't':
        if (current - start > 1) {
            switch (start[1]) {
            case 'h':
                return checkKeyword(2, 2, "is", TokenType::THIS);
            case 'r':
                return checkKeyword(2, 2, "ue", TokenType::TRUE);
            }
        }
        break;
    case 'v':
        return checkKeyword(1, 2, "ar", TokenType::VAR);
    case 'w':
        return checkKeyword(1, 4, "hile", TokenType::WHILE);
    }

    return TokenType::IDENTIFIER;
}

Token Scanner::identifier() {
    while (isAlpha(peek()) || isDigit(peek())) {
        advance();
    }
    return makeToken(identifierType());
}

Token Scanner::number() {
    while (isDigit(peek())) {
        advance();
    }

    // Look for a fractional part.
    if (peek() == '.' && isDigit(peekNext())) {
        // Consume the ".".
        advance();

        while (isDigit(peek())) {
            advance();
        }
    }

    return makeToken(TokenType::NUMBER);
}

Token Scanner::string() {
    while (peek() != '"' && !isAtEnd()) {
        if (peek() == '\n') {
            line++;
        }
        advance();
    }

    if (isAtEnd()) {
        return errorToken("Unterminated string.");
    }

    // The closing quote.
    advance();
    return makeToken(TokenType::STRING);
}

Token Scanner::scanToken() {
    skipWhitespace();
    start = current;

    if (isAtEnd()) {
        return makeToken(TokenType::EOFS);
    }

    char c = advance();
    if (isAlpha(c)) {
        return identifier();
    }
    if (isDigit(c)) {
        return number();
    }

    switch (c) {
    case '(':
        return makeToken(TokenType::LEFT_PAREN);
    case ')':
        return makeToken(TokenType::RIGHT_PAREN);
    case '{':
        return makeToken(TokenType::LEFT_BRACE);
    case '}':
        return makeToken(TokenType::RIGHT_BRACE);
    case ';':
        return makeToken(TokenType::SEMICOLON);
    case ',':
        return makeToken(TokenType::COMMA);
    case '.':
        return makeToken(TokenType::DOT);
    case '-':
        return makeToken(TokenType::MINUS);
    case '+':
        return makeToken(TokenType::PLUS);
    case '/':
        return makeToken(TokenType::SLASH);
    case '*':
        return makeToken(TokenType::ASTÉRIX);
    case '!':
        return makeToken(match('=') ? TokenType::BANG_EQUAL : TokenType::BANG);
    case '=':
        return makeToken(match('=') ? TokenType::EQUAL_EQUAL : TokenType::EQUAL);
    case '<':
        return makeToken(match('=') ? TokenType::LESS_EQUAL : TokenType::LESS);
    case '>':
        return makeToken(match('=') ? TokenType::GREATER_EQUAL : TokenType::GREATER);
    case '"':
        return string();
    default:
        return errorToken("Unexpected character.");
    }
}
