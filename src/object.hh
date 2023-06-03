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

    Value       receiver{};
    ObjClosure *method{};
};

constexpr ObjType obj_type(Value value) {
    return (as<Obj *>(value)->get_type());
}

constexpr bool isObjType(Value value, ObjType type) {
    return is<Obj>(value) && as<Obj *>(value)->get_type() == type;
}

template <> constexpr bool is<ObjBoundMethod>(Value value) {
    return isObjType(value, OBJ_BOUND_METHOD);
}

template <> constexpr bool is<ObjClass>(Value value) {
    return isObjType(value, OBJ_CLASS);
}

template <> constexpr bool is<ObjClosure>(Value value) {
    return isObjType(value, OBJ_CLOSURE);
}

template <> constexpr bool is<ObjFunction>(Value value) {
    return isObjType(value, OBJ_FUNCTION);
}

template <> constexpr bool is<ObjInstance>(Value value) {
    return isObjType(value, OBJ_INSTANCE);
}

template <> constexpr bool is<ObjNative>(Value value) {
    return isObjType(value, OBJ_NATIVE);
}

template <> constexpr bool is<ObjString>(Value value) {
    return isObjType(value, OBJ_STRING);
}

template <> inline ObjBoundMethod *as<ObjBoundMethod *>(Value value) {
    return reinterpret_cast<ObjBoundMethod *>(as<Obj *>(value));
}

template <> inline ObjClass *as<ObjClass *>(Value value) {
    return reinterpret_cast<ObjClass *>(as<Obj *>(value));
}

template <> inline ObjClosure *as<ObjClosure *>(Value value) {
    return reinterpret_cast<ObjClosure *>(as<Obj *>(value));
}

template <> inline ObjFunction *as<ObjFunction *>(Value value) {
    return reinterpret_cast<ObjFunction *>(as<Obj *>(value));
}

template <> inline ObjInstance *as<ObjInstance *>(Value value) {
    return reinterpret_cast<ObjInstance *>(as<Obj *>(value));
}

template <> inline NativeFn as<NativeFn>(Value value) {
    return reinterpret_cast<ObjNative *>(as<Obj *>(value))->function;
}

template <> inline ObjString *as<ObjString *>(Value value) {
    return reinterpret_cast<ObjString *>(as<Obj *>(value));
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
