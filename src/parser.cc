//
// ALOX-CC
//

#include <iostream>

#include <fmt/core.h>

#include "parser.hh"

inline constexpr auto buf_len = 255;

void Parser::errorAt(Token *token, std::string_view message) {
    if (panicMode) {
        return;
    }
    panicMode = true;
    std::cerr << fmt::format("[line {}] Error", token->line);

    if (token->type == TOKEN_EOF) {
        std::cerr << " at end";
    } else if (token->type == TOKEN_ERROR) {
        // Nothing.
    } else {
        std::array<char, buf_len> buf{};
        (void)snprintf(buf.data(), buf_len, " at '%.*s'", token->length, token->start);
        std::cerr << buf.data();
    }
    std::cerr << fmt::format(": {}\n", message);
    hadError = true;
}

void Parser::advance() {
    previous = current;

    for (;;) {
        current = scanner->scanToken();
        if (current.type != TOKEN_ERROR) {
            break;
        }
        errorAtCurrent(current.start);
    }
}

void Parser::consume(TokenType type, std::string_view message) {
    if (current.type == type) {
        advance();
        return;
    }
    errorAtCurrent(message);
}

bool Parser::match(TokenType type) {
    if (!check(type)) {
        return false;
    }
    advance();
    return true;
}
