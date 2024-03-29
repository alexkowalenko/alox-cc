//
// ALOX-CC
//

#include <iostream>

#include "fmt/core.h"

#include "error.hh"

namespace alox {

void ErrorManager::errorAt(size_t line, std::string_view message) {
    if (panicMode) {
        return;
    }
    panicMode = true;
    cerr << fmt::format("[line {}] Error", line);

    cerr << fmt::format(": {}\n", message);
    hadError = true;
}

void ErrorManager::errorAt(Token *token, std::string_view message) {
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

} // namespace lox