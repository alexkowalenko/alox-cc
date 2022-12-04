//
// ALOX-CC
//
// Copyright © Alex Kowalenko 2022.
//

#pragma once

#include "ast_base.hh"
#include "object.hh"
#include "scanner.hh"

constexpr ObjType AST_Binary = 107;

class Binary {
  public:
    Obj obj;

    Expr     *left{nullptr};
    Expr     *right{nullptr};
    TokenType token;
};

inline Binary *newBinary() {
    auto *ast = gc.allocateObject<Binary>(AST_Binary);
    return ast;
}

constexpr bool IS_Binary(Obj *obj) {
    return obj->type == AST_Binary;
}

inline Binary *AS_Binary(Obj *obj) {
    return reinterpret_cast<Binary *>(obj);
}
