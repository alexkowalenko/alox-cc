//
// ALOX-CC
//

#include "error.hh"

#include <format>
#include <fstream>
namespace alox {

void ErrorManager::errorAt(size_t line, std::string_view message) {
    if (panicMode) {
        return;
    }
    panicMode = true;
    cerr << std::format("[line {}] Error", line);

    cerr << std::format(": {}\n", message);
    hadError = true;
}

void ErrorManager::errorAt(Token *token, std::string_view message) {
    if (panicMode) {
        return;
    }
    panicMode = true;
    cerr << std::format("[line {}] Error", token->line);

    if (token->type == TokenType::EOFS) {
        cerr << " at end";
    } else if (token->type == TokenType::ERROR) {
        // Nothing.
    } else {
        cerr << std::format(" at '{:s}'", token->text);
    }
    cerr << std::format(": {}\n", message);
    hadError = true;
}

} // namespace alox