//
// ALOX-CC
//
// Copyright © Alex Kowalenko 2022.
//

#pragma once

#include "ast_base.hh"
#include "object.hh"

constexpr ObjType AST_Number = 105;

class Number {
  public:
    Obj obj;

    double value;
};

inline Number *newNumber() {
    auto *ast = gc.allocateObject<Number>(AST_Number);
    return ast;
}

constexpr bool IS_Number(Obj *obj) {
    return obj->type == AST_Number;
}

inline Number *AS_Number(Obj *obj) {
    return reinterpret_cast<Number *>(obj);
}
