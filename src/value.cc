//
// ALOX-CC
//

#include <iostream>

#include <fmt/core.h>

#include "memory.hh"
#include "object.hh"
#include "value.hh"

namespace alox {

void printValue(std::ostream &os, Value value) {
#ifdef NAN_BOXING
    if (IS_BOOL(value)) {
        os << (AS_BOOL(value) ? "true" : "false");
    } else if (IS_NIL(value)) {
        os << "nil";
    } else if (IS_NUMBER(value)) {
        os << fmt::format("{:g}", AS_NUMBER(value));
    } else if (IS_OBJ(value)) {
        printObject(os, value);
    }
#else
    switch (value.type) {
    case VAL_BOOL:
        os << (AS_BOOL(value) ? "true" : "false");
        break;
    case VAL_NIL:
        os << "nil";
        break;
    case VAL_NUMBER:
        os << fmt::format("{:g}", AS_NUMBER(value));
        break;
    case VAL_OBJ:
        printObject(os, value);
        break;
    }
#endif
}

bool valuesEqual(Value a, Value b) {
#ifdef NAN_BOXING
    if (IS_NUMBER(a) && IS_NUMBER(b)) {
        return AS_NUMBER(a) == AS_NUMBER(b);
    }
    if (IS_STRING(a) && IS_STRING(b)) {
        // fmt::print("{:d} {:d}\n", AS_STRING(a)->str.size(), AS_STRING(b)->str.size());
        return AS_STRING(a)->str == AS_STRING(b)->str;
    }
    return a == b;
#else
    if (a.type != b.type) {
        return false;
    }
    switch (a.type) {
    case VAL_BOOL:
        return AS_BOOL(a) == AS_BOOL(b);
    case VAL_NIL:
        return true;
    case VAL_NUMBER:
        return AS_NUMBER(a) == AS_NUMBER(b);
    case VAL_OBJ: {
        if (IS_STRING(a) && IS_STRING(b)) {
            return AS_STRING(a)->str == AS_STRING(b)->str;
        }
        return AS_OBJ(a) == AS_OBJ(b);
    }
    default:
        return false; // Unreachable.
    }
#endif
}

} // namespace lox