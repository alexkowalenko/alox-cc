//
// ALOX-CC
//

#pragma once

#include <string>

#include "chunk.hh"
#include "common.hh"
#include "table.hh"
#include "value.hh"

namespace alox {

using ObjType = uint8_t;

constexpr ObjType OBJ_BOUND_METHOD = 0;
constexpr ObjType OBJ_CLASS = 1;
constexpr ObjType OBJ_CLOSURE = 2;
constexpr ObjType OBJ_FUNCTION = 3;
constexpr ObjType OBJ_INSTANCE = 4;
constexpr ObjType OBJ_NATIVE = 5;
constexpr ObjType OBJ_STRING = 6;
constexpr ObjType OBJ_UPVALUE = 7;

class Obj {
  public:
    explicit Obj(ObjType t) : type(t){};

    [[nodiscard]] ObjType get_type() const { return type; }

  private:
    ObjType type;
};

class ObjFunction : public Obj {
  public:
    ObjFunction() : Obj(OBJ_FUNCTION){};

    int        arity{};
    int        upvalueCount{};
    Chunk      chunk;
    ObjString *name{};
};

using NativeFn = Value (*)(int, Value const *);

class ObjNative : public Obj {
  public:
    ObjNative() : Obj(OBJ_NATIVE){};

    NativeFn function{};
};

class ObjString : public Obj {
  public:
    ObjString() : Obj(OBJ_STRING){};

    std::string str;
    uint32_t    hash{};
};

class ObjUpvalue : public Obj {
  public:
    ObjUpvalue() : Obj(OBJ_UPVALUE){};

    Value      *location{};
    Value       closed{};
    ObjUpvalue *next{};
};

class ObjClosure : public Obj {
  public:
    ObjClosure() : Obj(OBJ_CLOSURE){};

    ObjFunction *function{};
    ObjUpvalue **upvalues{};
    int          upvalueCount{};
};

class ObjClass : public Obj {
  public:
    ObjClass() : Obj(OBJ_CLASS){};

    ObjString *name{};
    Table      methods;
};

class ObjInstance : public Obj {
  public:
    ObjInstance() : Obj(OBJ_INSTANCE){};

    ObjClass *klass{};
    Table     fields; // [fields]
};

class ObjBoundMethod : public Obj {
  public:
    ObjBoundMethod() : Obj(OBJ_BOUND_METHOD){};

    Value       receiver;
    ObjClosure *method;
};

constexpr ObjType OBJ_TYPE(Value value) {
    return (AS_OBJ(value)->get_type());
}

constexpr bool isObjType(Value value, ObjType type) {
    return IS_OBJ(value) && AS_OBJ(value)->get_type() == type;
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

inline ObjBoundMethod *AS_BOUND_METHOD(Value value) {
    return reinterpret_cast<ObjBoundMethod *>(AS_OBJ(value));
}

inline ObjClass *AS_CLASS(Value value) {
    return reinterpret_cast<ObjClass *>(AS_OBJ(value));
}

inline ObjClosure *AS_CLOSURE(Value value) {
    return reinterpret_cast<ObjClosure *>(AS_OBJ(value));
}

inline ObjFunction *AS_FUNCTION(Value value) {
    return reinterpret_cast<ObjFunction *>(AS_OBJ(value));
}

inline ObjInstance *AS_INSTANCE(Value value) {
    return reinterpret_cast<ObjInstance *>(AS_OBJ(value));
}

inline NativeFn AS_NATIVE(Value value) {
    return reinterpret_cast<ObjNative *>(AS_OBJ(value))->function;
}

inline ObjString *AS_STRING(Value value) {
    return reinterpret_cast<ObjString *>(AS_OBJ(value));
}

inline const std::string &AS_CSTRING(Value value) {
    return reinterpret_cast<ObjString *>(AS_OBJ(value))->str;
}

ObjBoundMethod *newBoundMethod(Value receiver, ObjClosure *method);
ObjClass       *newClass(ObjString *name);
ObjClosure     *newClosure(ObjFunction *function);
ObjFunction    *newFunction();
ObjInstance    *newInstance(ObjClass *klass);
ObjNative      *newNative(NativeFn function);
ObjString      *newString(std::string const &s);
ObjUpvalue     *newUpvalue(Value *slot);
void            printObject(std::ostream &os, Value value);

} // namespace alox
