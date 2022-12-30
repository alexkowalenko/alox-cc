//
// ALOX-CC
//

#pragma once

#include <memory>
#include <ostream>
#include <string_view>

#include "ast/includes.hh"
#include "context.hh"
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
    Parser(Scanner &s, ErrorManager &err) : scanner(s), err(err){};

    Declaration *parse();

    Obj      *declaration();
    VarDec   *varDeclaration();
    FunctDec *funDeclaration(FunctionType type = TYPE_FUNCTION);
    ClassDec *classDeclaration();
    FunctDec *method();

    Statement *statement();
    If        *if_stat();
    While     *while_stat();
    For       *for_stat();
    Print     *printStatement();
    Return    *return_stat();
    Break     *break_stat(TokenType t);
    Expr      *exprStatement();

    Block *block();

    Expr *expr();
    Expr *parsePrecedence(Precedence precedence);

    Expr *binary(Expr *left, bool /*canAssign*/);
    Expr *assign(Expr *left, bool /*canAssign*/);
    Expr *call(Expr *left, bool /*canAssign*/);
    Expr *dot(Expr *left, bool /*canAssign*/);

    Expr *grouping(bool /*canAssign*/);
    Expr *unary(bool /*canAssign*/);
    Expr *identifier(bool /*canAssign*/);
    Expr *primary(bool /*canAssign*/);
    Expr *string(bool /*canAssign*/);
    Expr *number(bool /*canAssign*/);
    Expr *super_(bool /*canAssign*/);
    Expr *this_(bool /*canAssign*/);

    Identifier *ident();

    void argumentList(std::vector<Expr *> &args);

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
    Scanner      &scanner;
    ErrorManager &err;
};
