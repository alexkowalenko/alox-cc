//
// ALOX-CC
//

#pragma once

#include <ostream>
#include <stddef.h>
#include <string.h>

#include "common.hh"

struct ObjString;
struct Obj;

#ifdef NAN_BOXING

constexpr uint64_t SIGN_BIT = ((uint64_t)0x8000000000000000);
constexpr uint64_t QNAN = ((uint64_t)0x7ffc000000000000);

constexpr auto TAG_NIL = 1;   // 01.
constexpr auto TAG_FALSE = 2; // 10.
constexpr auto TAG_TRUE = 3;  // 11.

using Value = uint64_t;

constexpr Value FALSE_VAL = ((Value)(uint64_t)(QNAN | TAG_FALSE));
constexpr Value TRUE_VAL = ((Value)(uint64_t)(QNAN | TAG_TRUE));
constexpr Value NIL_VAL = ((Value)(uint64_t)(QNAN | TAG_NIL));

constexpr bool IS_BOOL(Value value) {
    return (((value) | 1) == TRUE_VAL);
}

constexpr bool IS_NIL(Value value) {
    return ((value) == NIL_VAL);
}

constexpr bool IS_NUMBER(Value value) {
    return (((value)&QNAN) != QNAN);
}

constexpr bool IS_OBJ(Value value) {
    return (((value) & (QNAN | SIGN_BIT)) == (QNAN | SIGN_BIT));
}

constexpr bool AS_BOOL(Value value) {
    return ((value) == TRUE_VAL);
}

static inline double valueToNum(Value value) {
    double num;
    memcpy(&num, &value, sizeof(Value));
    return num;
}

static inline Value numToValue(double num) {
    Value value;
    memcpy(&value, &num, sizeof(double));
    return value;
}

constexpr double AS_NUMBER(Value value) {
    return valueToNum(value);
}

constexpr Value NUMBER_VAL(double num) {
    return numToValue(num);
}

constexpr Value BOOL_VAL(bool b) {
    return ((b) ? TRUE_VAL : FALSE_VAL);
}

inline Obj *AS_OBJ(Value value) {
    return (Obj *)(uintptr_t)((value) & ~(SIGN_BIT | QNAN));
}

template <typename T> Value OBJ_VAL(T *obj) {
    return static_cast<Value>(SIGN_BIT | QNAN |
                              static_cast<uint64_t>(reinterpret_cast<uintptr_t>(obj)));
}

#else

enum ValueType {
    VAL_BOOL,
    VAL_NIL, // [user-types]
    VAL_NUMBER,
    VAL_OBJ
};

struct Obj;

struct Value {
    ValueType type;
    union {
        bool   boolean;
        double number;
        Obj   *obj;
    } as; // [as]
};

constexpr bool IS_BOOL(Value value) {
    return value.type == VAL_BOOL;
}

constexpr bool IS_NIL(Value value) {
    return value.type == VAL_NIL;
}

constexpr bool IS_NUMBER(Value value) {
    return value.type == VAL_NUMBER;
}

constexpr bool IS_OBJ(Value value) {
    return value.type == VAL_OBJ;
}

constexpr Obj *AS_OBJ(Value value) {
    return value.as.obj;
}

constexpr bool AS_BOOL(Value value) {
    return value.as.boolean;
}

constexpr double AS_NUMBER(Value value) {
    return value.as.number;
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc99-extensions"
constexpr Value BOOL_VAL(bool value) {
    return (Value){VAL_BOOL, {.boolean = value}};
}

constexpr Value NIL_VAL = ((Value){VAL_NIL, {.number = 0}});

constexpr Value NUMBER_VAL(double value) {
    return (Value){VAL_NUMBER, {.number = value}};
}

template <typename T> inline Value OBJ_VAL(T *object) {
    return Value({VAL_OBJ, {.obj = (Obj *)(object)}});
}

#pragma clang diagnostic pop

#endif

bool valuesEqual(Value a, Value b);
void printValue(std::ostream &os, Value value);
