//
// ALOX-CC
//

#pragma once

#include <memory>
#include <ostream>
#include <string_view>

#include "ast/includes.hh"
#include "error.hh"
#include "scanner.hh"

enum class Precedence {
    NONE,
    ASSIGNMENT, // =
    OR,         // or
    AND,        // and
    EQUALITY,   // == !=
    COMPARISON, // < > <= >=
    TERM,       // + -
    FACTOR,     // * /
    UNARY,      // ! -
    CALL,       // . ()
    PRIMARY
};

class Parser;
using ParseFn = std::function<Expr *(Parser *, bool)>;
using ParseFnBin = std::function<Expr *(Parser *, Expr *, bool)>;

struct ParseRule {
    ParseFn    prefix;
    ParseFnBin infix;
};

class Parser {
  public:
    Parser(Scanner &s, Error &err) : scanner(s), err(err){};

    Declaration *parse();

    Obj *    declaration();
    VarDec *varDeclaration();

    Statement *statement();
    Print     *printStatement();
    Expr      *exprStatement();

    Expr *expr();
    Expr *parsePrecedence(Precedence precedence);

    Expr *binary(Expr *left, bool /*canAssign*/);
    Expr *grouping(bool /*canAssign*/);
    Expr *unary(bool /*canAssign*/);
    Expr *identifier(bool /*canAssign*/);
    Expr *primary(bool /*canAssign*/);
    Expr *string(bool /*canAssign*/);
    Expr *number(bool /*canAssign*/);

    Block *block();

    Identifier *ident();

    static ParseRule const *getRule(TokenType type);

    constexpr void error(std::string_view message) { err.errorAt(&previous, message); }
    constexpr void errorAtCurrent(std::string_view message) {
        err.errorAt(&current, message);
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

  private:
    Scanner &scanner;
    Error   &err;
};
