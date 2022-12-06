//
// ALOX-CC
//
// Copyright © Alex Kowalenko 2022.
//

#pragma once

#include "memory.hh"
#include "object.hh"

class Declaration;
class Statement;
class Expr;
class Primary;
class Number;
class Identifier;

template <typename T> Obj *OBJ_AST(T *obj) {
    return reinterpret_cast<Obj *>(obj);
}