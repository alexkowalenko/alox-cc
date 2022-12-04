//
// ALOX-CC
//
// Copyright © Alex Kowalenko 2022.
//

#pragma once

#include "ast_base.hh"
#include "object.hh"

constexpr ObjType AST_Expr = 102;

class Expr {
  public:
    Obj obj;

    Obj *expr{nullptr};
};

inline Expr *newExpr() {
    auto *ast = gc.allocateObject<Expr>(AST_Expr);
    return ast;
}

constexpr bool IS_Expr(Obj *obj) {
    return obj->type == AST_Expr;
}

inline Expr *AS_Expr(Obj *obj) {
    return reinterpret_cast<Expr *>(obj);
}
