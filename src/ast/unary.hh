//
// ALOX-CC
//
// Copyright © Alex Kowalenko 2022.
//

#pragma once

#include "ast_base.hh"
#include "object.hh"
#include "scanner.hh"

constexpr ObjType AST_Unary = 106;

class Unary {
  public:
    Obj obj;

    Expr     *expr{nullptr};
    TokenType token;
};

inline Unary *newUnary() {
    auto *ast = gc.allocateObject<Unary>(AST_Unary);
    return ast;
}

constexpr bool IS_Unary(Obj *obj) {
    return obj->type == AST_Unary;
}

inline Unary *AS_Unary(Obj *obj) {
    return reinterpret_cast<Unary *>(obj);
}
