//
// ALOX-CC
//
// Copyright © Alex Kowalenko 2022.
//

#pragma once

#include "ast/assign.hh"
#include "ast/block.hh"
#include "ast/boolean.hh"
#include "ast/identifier.hh"
#include "ast/includes.hh"
#include "ast/vardec.hh"
#include "ast_base.hh"
#include "context.hh"
#include <cstddef>
#include <ostream>

namespace alox {

class AST_Printer {
  public:
    explicit AST_Printer(std::ostream &os, char nl = '\n', size_t indent = 4)
        : os(os), NL(nl), indent(indent){};

    void print(Declaration *);

  private:
    void declaration(Declaration *ast);
    void decs_statement(Obj *);
    void varDec(VarDec *ast);
    void funDec(FunctDec *ast, FunctionType type = TYPE_FUNCTION);
    void classDec(ClassDec *ast);
    void statement(Statement *s);
    void if_stat(If *s);
    void while_stat(While *);
    void for_stat(For *);
    void return_stat(Return *);
    void break_stat(Break *);
    void printStatement(Print *s);
    void block(Block *s);
    void exprStatement(Expr *s);
    void expr(Expr *expr);
    void binary(Binary *ast);
    void assign(Assign *ast);
    void call(Call *ast);
    void dot(Dot *ast);
    void unary(Unary *ast);
    void identifier(Identifier *ast);
    void boolean(Boolean *expr);
    void number(Number *num);
    void string(String *num);
    void this_(This *num);

    void args(const std::vector<Expr *> &args);

    std::ostream &os;
    const char    NL{};
    size_t        indent{4};
};

} // namespace lox