//
// ALOX-CC
//
// Copyright © Alex Kowalenko 2022.
//

#pragma once

#include "ast_base.hh"
#include "object.hh"

constexpr ObjType AST_Declaration = 100;

class Declaration {
  public:
    Obj obj;

    std::vector<Statement *> stats;
};

inline Declaration *newDeclaration() {
    auto *decl = gc.allocateObject<Declaration>(AST_Declaration);
    return decl;
}

constexpr bool IS_Declaration(Obj *obj) {
    return obj->type == AST_Declaration;
}

inline Declaration *AS_Declaration(Obj *obj) {
    return reinterpret_cast<Declaration *>(obj);
}
