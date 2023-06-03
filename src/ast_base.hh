//
// ALOX-CC
//
// Copyright © Alex Kowalenko 2022.
//

#pragma once

#include "memory.hh"
#include "object.hh"

namespace alox {

class Declaration;
class Statement;
class Expr;
class Primary;
class Number;
class Identifier;

template <typename T> Obj *OBJ_AST(T *obj) {
    return reinterpret_cast<Obj *>(obj);
}

constexpr auto START_AST = 100;

class AST_Base : public Obj {
  public:
    AST_Base(ObjType type, int l) : Obj(type), line(l) {}

    int get_line() const { return line; }

  private:
    int line;
};

template <typename T> bool is(Obj *obj);
template <typename T> T   *as(Obj *obj);

} // namespace alox
