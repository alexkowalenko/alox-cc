//
// ALOX-CC
//
// Copyright © Alex Kowalenko 2022.
//

#pragma once

#include "ast_base.hh"
#include "object.hh"

constexpr ObjType AST_Statement = 101;

class Statement {
  public:
    Obj obj;

    Obj *stat;
};

inline Statement *newStatement() {
    auto *ast = gc.allocateObject<Statement>(AST_Statement);
    return ast;
}

constexpr bool IS_Statement(Obj *obj) {
    return obj->type == AST_Statement;
}

inline Statement *AS_Statement(Obj *obj) {
    return reinterpret_cast<Statement *>(obj);
}
