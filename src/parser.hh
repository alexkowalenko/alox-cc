//
// ALOX-CC
//

#pragma once

#include <memory>
#include <ostream>
#include <string_view>

#include "ast/includes.hh"
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
    Parser(Scanner &s, std::ostream &err) : scanner(s), err(err){};

    Declaration *parse();
    void         declaration(Declaration *);
    VarDec      *varDeclaration();

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

    Identifier *ident();

    static ParseRule const *getRule(TokenType type);

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
    Scanner      &scanner;
    std::ostream &err;
};
