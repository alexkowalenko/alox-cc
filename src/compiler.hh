//
// ALOX-CC
//

#pragma once

#include "object.hh"
#include "vm.hh"

ObjFunction *compile(const char *source);
void         markCompilerRoots();
