//
// ALOX-CC
//

#pragma once

#include "scanner.hh"
#include <memory>
#include <string_view>

class Parser {
  public:
    Parser(std::unique_ptr<Scanner> &s) : scanner(s){};

    void errorAt(Token *token, std::string_view message);

    constexpr void error(std::string_view message) { errorAt(&previous, message); }
    constexpr void errorAtCurrent(std::string_view message) {
        errorAt(&current, message);
    }

    void                         advance();
    void                         consume(TokenType type, std::string_view message);
    [[nodiscard]] constexpr bool check(TokenType type) const {
        return current.type == type;
    }
    bool match(TokenType type);

    Token current;
    Token previous;

    bool panicMode{false};
    bool hadError{false};

  private:
    std::unique_ptr<Scanner> &scanner;
};
