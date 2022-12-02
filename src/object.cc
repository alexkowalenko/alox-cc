//
// ALOX-CC
//

#include <cstring>
#include <functional>
#include <iostream>

#include <fmt/core.h>
#include <string_view>

#include "memory.hh"
#include "object.hh"
#include "table.hh"
#include "value.hh"

ObjBoundMethod *newBoundMethod(Value receiver, ObjClosure *method) {
    auto *bound = gc.allocateObject<ObjBoundMethod>(ObjType::BOUND_METHOD);
    bound->receiver = receiver;
    bound->method = method;
    return bound;
}

ObjClass *newClass(ObjString *name) {
    auto *klass = gc.allocateObject<ObjClass>(ObjType::CLASS);
    klass->name = name; // [klass]
    return klass;
}

ObjClosure *newClosure(ObjFunction *function) {
    auto **upvalues = gc.allocate_array<ObjUpvalue *>(function->upvalueCount);
    for (int i = 0; i < function->upvalueCount; i++) {
        upvalues[i] = nullptr;
    }

    auto *closure = gc.allocateObject<ObjClosure>(ObjType::CLOSURE);
    closure->function = function;
    closure->upvalues = upvalues;
    closure->upvalueCount = function->upvalueCount;
    return closure;
}

ObjFunction *newFunction() {
    auto *function = gc.allocateObject<ObjFunction>(ObjType::FUNCTION);
    function->arity = 0;
    function->upvalueCount = 0;
    function->name = nullptr;
    return function;
}

ObjInstance *newInstance(ObjClass *klass) {
    auto *instance = gc.allocateObject<ObjInstance>(ObjType::INSTANCE);
    instance->klass = klass;
    return instance;
}

ObjNative *newNative(NativeFn function) {
    auto *native = gc.allocateObject<ObjNative>(ObjType::NATIVE);
    native->function = function;
    return native;
}

inline uint32_t hashString(const std::string_view &s) {
    uint32_t hash = 2166136261u;
    for (auto c : s) {
        hash ^= (uint8_t)c;
        hash *= 16777619;
    }
    return hash;
}

ObjString *newString(std::string const &s) {
    auto *string = gc.allocateObject<ObjString>(ObjType::STRING);
    string->str = s;
    string->hash = hashString(s);
    return string;
}

ObjUpvalue *newUpvalue(Value *slot) {
    auto *upvalue = gc.allocateObject<ObjUpvalue>(ObjType::UPVALUE);
    upvalue->closed = NIL_VAL;
    upvalue->location = slot;
    upvalue->next = nullptr;
    return upvalue;
}

static void printFunction(ObjFunction *function) {
    if (function->name == nullptr) {
        std::cout << "<script>";
        return;
    }
    fmt::print("<fn {}>", function->name->str);
}

void printObject(Value value) {
    switch (OBJ_TYPE(value)) {
    case ObjType::BOUND_METHOD:
        printFunction(AS_BOUND_METHOD(value)->method->function);
        break;
    case ObjType::CLASS:
        fmt::print("{}", AS_CLASS(value)->name->str);
        break;
    case ObjType::CLOSURE:
        printFunction(AS_CLOSURE(value)->function);
        break;
    case ObjType::FUNCTION:
        printFunction(AS_FUNCTION(value));
        break;
    case ObjType::INSTANCE:
        fmt::print("{} instance", AS_INSTANCE(value)->klass->name->str);
        break;
    case ObjType::NATIVE:
        std::cout << "<native fn>";
        break;
    case ObjType::STRING:
        fmt::print("{}", AS_CSTRING(value));
        break;
    case ObjType::UPVALUE:
        std::cout << "upvalue";
        break;
    }
}
