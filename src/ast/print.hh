//
// ALOX-CC
//
// Copyright © Alex Kowalenko 2022.
//

#pragma once

#include "ast_base.hh"
#include "object.hh"

constexpr ObjType AST_Print = 106;

class Print {
  public:
    Obj obj;

    Expr *expr;
};

inline Print *newPrint() {
    auto *ast = gc.allocateObject<Print>(AST_Print);
    return ast;
}

constexpr bool IS_Print(Obj *obj) {
    return obj->type == AST_Print;
}

inline Print *AS_Print(Obj *obj) {
    return reinterpret_cast<Print *>(obj);
}