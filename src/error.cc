//
// ALOX-CC
//

#include <iostream>

#include "fmt/core.h"

#include "error.hh"

void Error::errorAt(int line, std::string_view message) {
    if (panicMode) {
        return;
    }
    panicMode = true;
    cerr << fmt::format("[line {}] Error", line);

    cerr << fmt::format(": {}\n", message);
    hadError = true;
}

void Error::errorAt(Token *token, std::string_view message) {
    if (panicMode) {
        return;
    }
    panicMode = true;
    cerr << fmt::format("[line {}] Error", token->line);

    if (token->type == TokenType::EOFS) {
        cerr << " at end";
    } else if (token->type == TokenType::ERROR) {
        // Nothing.
    } else {
        cerr << fmt::format(" at '{:s}'", token->text);
    }
    cerr << fmt::format(": {}\n", message);
    hadError = true;
}