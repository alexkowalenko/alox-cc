//
// ALOX-CC
//

#pragma once

#include <memory>
#include <string_view>

#include "scanner.hh"
#include <ast/declaration.hh>

class Parser {
  public:
    Parser(Scanner &s) : scanner(s){};

    Declaration *parse();
    void         declaration(Declaration *);
    Statement   *statement();
    Print       *printStatement();
    Expr        *exprStatement();
    Expr        *expr();
    Primary     *primary();
    Number      *number();

    void           errorAt(Token *token, std::string_view message);
    constexpr void error(std::string_view message) { errorAt(&previous, message); }
    constexpr void errorAtCurrent(std::string_view message) {
        errorAt(&current, message);
    }
    void synchronize();

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
    Scanner &scanner;
};
