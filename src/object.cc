//
// ALOX-CC
//

#include <stdio.h>
#include <string.h>

#include "memory.hh"
#include "object.hh"
#include "table.hh"
#include "value.hh"

template <typename T> constexpr T *allocate_obj(ObjType objectType) {
    return (T *)gc.allocateObject(sizeof(T), objectType);
}

ObjBoundMethod *newBoundMethod(Value receiver, ObjClosure *method) {
    auto *bound = allocate_obj<ObjBoundMethod>(OBJ_BOUND_METHOD);
    bound->receiver = receiver;
    bound->method = method;
    return bound;
}

ObjClass *newClass(ObjString *name) {
    auto *klass = gc.allocateObject<ObjClass>(OBJ_CLASS);
    klass->name = name; // [klass]
    klass->methods.init();
    return klass;
}

ObjClosure *newClosure(ObjFunction *function) {
    auto **upvalues = allocate<ObjUpvalue *>(function->upvalueCount);
    for (int i = 0; i < function->upvalueCount; i++) {
        upvalues[i] = nullptr;
    }

    auto *closure = allocate_obj<ObjClosure>(OBJ_CLOSURE);
    closure->function = function;
    closure->upvalues = upvalues;
    closure->upvalueCount = function->upvalueCount;
    return closure;
}

ObjFunction *newFunction() {
    auto *function = allocate_obj<ObjFunction>(OBJ_FUNCTION);
    function->arity = 0;
    function->upvalueCount = 0;
    function->name = nullptr;
    function->chunk.init();
    return function;
}

ObjInstance *newInstance(ObjClass *klass) {
    auto *instance = allocate_obj<ObjInstance>(OBJ_INSTANCE);
    instance->klass = klass;
    instance->fields.init();
    return instance;
}

ObjNative *newNative(NativeFn function) {
    auto *native = allocate_obj<ObjNative>(OBJ_NATIVE);
    native->function = function;
    return native;
}

ObjString *allocateString(char *chars, int length, uint32_t hash) {
    auto *string = allocate_obj<ObjString>(OBJ_STRING);
    string->length = length;
    string->chars = chars;
    string->hash = hash;

    gc.strings.set(string, NIL_VAL);
    return string;
}

static uint32_t hashString(const char *key, int length) {
    uint32_t hash = 2166136261u;
    for (int i = 0; i < length; i++) {
        hash ^= (uint8_t)key[i];
        hash *= 16777619;
    }
    return hash;
}

ObjString *takeString(char *chars, int length) {
    uint32_t   hash = hashString(chars, length);
    ObjString *interned = gc.strings.findString(chars, length, hash);
    if (interned != nullptr) {
        free_array<char>(chars, length + 1);
        return interned;
    }

    return allocateString(chars, length, hash);
}

ObjString *copyString(const char *chars, int length) {
    uint32_t   hash = hashString(chars, length);
    ObjString *interned = gc.strings.findString(chars, length, hash);
    if (interned != nullptr) {
        return interned;
    }

    char *heapChars = allocate<char>(length + 1);
    memcpy(heapChars, chars, length);
    heapChars[length] = '\0';
    return allocateString(heapChars, length, hash);
}

ObjUpvalue *newUpvalue(Value *slot) {
    auto *upvalue = allocate_obj<ObjUpvalue>(OBJ_UPVALUE);
    upvalue->closed = NIL_VAL;
    upvalue->location = slot;
    upvalue->next = nullptr;
    return upvalue;
}

static void printFunction(ObjFunction *function) {
    if (function->name == nullptr) {
        printf("<script>");
        return;
    }
    printf("<fn %s>", function->name->chars);
}

void printObject(Value value) {
    switch (OBJ_TYPE(value)) {
    case OBJ_BOUND_METHOD:
        printFunction(AS_BOUND_METHOD(value)->method->function);
        break;
    case OBJ_CLASS:
        printf("%s", AS_CLASS(value)->name->chars);
        break;
    case OBJ_CLOSURE:
        printFunction(AS_CLOSURE(value)->function);
        break;
    case OBJ_FUNCTION:
        printFunction(AS_FUNCTION(value));
        break;
    case OBJ_INSTANCE:
        printf("%s instance", AS_INSTANCE(value)->klass->name->chars);
        break;
    case OBJ_NATIVE:
        printf("<native fn>");
        break;
    case OBJ_STRING:
        printf("%s", AS_CSTRING(value));
        break;
    case OBJ_UPVALUE:
        printf("upvalue");
        break;
    }
}
