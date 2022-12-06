//
// ALOX-CC
//
// Copyright © Alex Kowalenko 2022.
//

#pragma once

#include "ast_base.hh"
#include "object.hh"
#include "scanner.hh"

constexpr ObjType AST_{{name}} = {{index}};

class {{name}} {
  public:
    Obj obj;
    int line{0};

    {{#instances}}
    {{{type}}}     {{name}};
    {{/instances}}
};

inline  {{name}} *new{{name}}(int l) {
    auto *ast = gc.allocateObject< {{name}}>(AST_{{name}});
    ast->line = l;
    return ast;
}

constexpr bool IS_{{name}}(Obj *obj) {
    return obj->type == AST_{{name}};
}

inline {{name}} *AS_{{name}}(Obj *obj) {
    return reinterpret_cast<{{name}} *>(obj);
}
