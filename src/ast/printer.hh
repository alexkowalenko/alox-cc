//
// ALOX-CC
//
// Copyright © Alex Kowalenko 2022.
//

#pragma once

#include "ast_base.hh"
#include "includes.hh"
#include <ostream>

class AST_Printer {
  public:
    AST_Printer(std::ostream &os) : os(os){};

    void print(Declaration *);

  private:
    void declaration(Declaration *ast);
    void statement(Statement *s);
    void printStatement(Print *s);
    void exprStatement(Expr *s);
    void expr(Expr *expr);
    void binary(Binary *ast);
    void unary(Unary *ast);
    void number(Number *num);

    std::ostream &os;
};