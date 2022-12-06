//
// ALOX-CC
//
// Copyright © Alex Kowalenko 2022.
//

#pragma once

#include "ast/boolean.hh"
#include "ast/identifier.hh"
#include "ast/includes.hh"
#include "ast/vardec.hh"
#include "ast_base.hh"
#include <ostream>

class AST_Printer {
  public:
    explicit AST_Printer(std::ostream &os, char nl = '\n') : os(os), NL(nl){};

    void print(Declaration *);

  private:
    void declaration(Declaration *ast);
    void varDec(VarDec *ast);
    void statement(Statement *s);
    void printStatement(Print *s);
    void exprStatement(Expr *s);
    void expr(Expr *expr);
    void binary(Binary *ast);
    void unary(Unary *ast);
    void identifier(Identifier *ast);
    void boolean(Boolean *expr);
    void number(Number *num);
    void string(String *num);

    std::ostream &os;
    const char    NL{};
};