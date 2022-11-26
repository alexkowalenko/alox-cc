//
// ALOX-CC
//

#pragma once

#include "chunk.hh"
#include "common.hh"
#include "table.hh"
#include "value.hh"

enum ObjType {
    OBJ_BOUND_METHOD,
    OBJ_CLASS,
    OBJ_CLOSURE,
    OBJ_FUNCTION,
    OBJ_INSTANCE,
    OBJ_NATIVE,
    OBJ_STRING,
    OBJ_UPVALUE
};

struct Obj {
    ObjType     type;
    bool        isMarked;
    struct Obj *next;
};

struct ObjFunction {
  public:
    Obj        obj;
    int        arity;
    int        upvalueCount;
    Chunk      chunk;
    ObjString *name;
};

using NativeFn = Value (*)(int, Value *);

struct ObjNative {
    Obj      obj;
    NativeFn function;
};

struct ObjString {
    Obj      obj;
    int      length;
    char    *chars;
    uint32_t hash;
};

struct ObjUpvalue {
    Obj                obj;
    Value             *location;
    Value              closed;
    struct ObjUpvalue *next;
};

struct ObjClosure {
    Obj          obj;
    ObjFunction *function;
    ObjUpvalue **upvalues;
    int          upvalueCount;
};

struct ObjClass {
    Obj        obj;
    ObjString *name;
    Table      methods;
};
struct ObjInstance {
    Obj       obj;
    ObjClass *klass;
    Table     fields; // [fields]
};

struct ObjBoundMethod {
    Obj         obj;
    Value       receiver;
    ObjClosure *method;
};

constexpr ObjType OBJ_TYPE(Value value) {
    return (AS_OBJ(value)->type);
}

constexpr bool isObjType(Value value, ObjType type) {
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

constexpr bool IS_BOUND_METHOD(Value value) {
    return isObjType(value, OBJ_BOUND_METHOD);
}

constexpr bool IS_CLASS(Value value) {
    return isObjType(value, OBJ_CLASS);
}

constexpr bool IS_CLOSURE(Value value) {
    return isObjType(value, OBJ_CLOSURE);
}

constexpr bool IS_FUNCTION(Value value) {
    return isObjType(value, OBJ_FUNCTION);
}

constexpr bool IS_INSTANCE(Value value) {
    return isObjType(value, OBJ_INSTANCE);
}

constexpr bool IS_NATIVE(Value value) {
    return isObjType(value, OBJ_NATIVE);
}

constexpr bool IS_STRING(Value value) {
    return isObjType(value, OBJ_STRING);
}

#define AS_BOUND_METHOD(value) (reinterpret_cast<ObjBoundMethod *>(AS_OBJ(value)))
#define AS_CLASS(value)        (reinterpret_cast<ObjClass *>(AS_OBJ(value)))
#define AS_CLOSURE(value)      (reinterpret_cast<ObjClosure *>(AS_OBJ(value)))
#define AS_FUNCTION(value)     (reinterpret_cast<ObjFunction *>(AS_OBJ(value)))
#define AS_INSTANCE(value)     (reinterpret_cast<ObjInstance *>(AS_OBJ(value)))
#define AS_NATIVE(value)       (reinterpret_cast<ObjNative *>(AS_OBJ(value))->function)
#define AS_STRING(value)       (reinterpret_cast<ObjString *>(AS_OBJ(value)))
#define AS_CSTRING(value)      (reinterpret_cast<ObjString *>(AS_OBJ(value))->chars)

ObjBoundMethod *newBoundMethod(Value receiver, ObjClosure *method);
ObjClass       *newClass(ObjString *name);
ObjClosure     *newClosure(ObjFunction *function);
ObjFunction    *newFunction();
ObjInstance    *newInstance(ObjClass *klass);
ObjNative      *newNative(NativeFn function);
ObjString      *takeString(char *chars, int length);
ObjString      *copyString(const char *chars, int length);
ObjUpvalue     *newUpvalue(Value *slot);
void            printObject(Value value);
