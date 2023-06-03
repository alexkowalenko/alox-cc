//
// ALOX-CC
//
// Copyright © Alex Kowalenko 2022.
//

#pragma once

#include "ast_base.hh"
#include "object.hh"
#include "scanner.hh"

namespace alox {

constexpr ObjType AST_{{name}} = {{index}};

class {{name}} : public AST_Base {
  public:
  
    {{name}}(int l) : AST_Base(AST_{{name}}, l) {};

    {{#instances}}
    {{{type}}}     {{name}};
    {{/instances}}
};

template <> constexpr bool is<{{name}}>(Obj *obj) {
    return obj->get_type() == AST_{{name}};
}

template <> inline {{name}}* as<{{name}}>(Obj *obj) {
    return reinterpret_cast<{{name}} *>(obj);
}

}