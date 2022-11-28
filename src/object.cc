//
// ALOX-CC
//

#include <stdio.h>
#include <string.h>

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
    klass->methods.init();
    return klass;
}

ObjClosure *newClosure(ObjFunction *function) {
    auto **upvalues =
        gc.allocate_array<ObjUpvalue *>(function->upvalueCount); // allocate array
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
    function->chunk.init();
    return function;
}

ObjInstance *newInstance(ObjClass *klass) {
    auto *instance = gc.allocateObject<ObjInstance>(ObjType::INSTANCE);
    instance->klass = klass;
    instance->fields.init();
    return instance;
}

ObjNative *newNative(NativeFn function) {
    auto *native = gc.allocateObject<ObjNative>(ObjType::NATIVE);
    native->function = function;
    return native;
}

ObjString *allocateString(char *chars, int length, uint32_t hash) {
    auto *string = gc.allocateObject<ObjString>(ObjType::STRING);
    string->length = length;
    string->chars = chars;
    string->hash = hash;

    gc.strings.set(string, NIL_VAL);
    return string;
}

static constexpr uint32_t hashString(const char *key, int length) {
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
        gc.delete_array<char>(chars, length + 1);
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

    char *heapChars = gc.allocate_array<char>(length + 1);
    memcpy(heapChars, chars, length);
    heapChars[length] = '\0';
    return allocateString(heapChars, length, hash);
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
        printf("<script>");
        return;
    }
    printf("<fn %s>", function->name->chars);
}

void printObject(Value value) {
    switch (OBJ_TYPE(value)) {
    case ObjType::BOUND_METHOD:
        printFunction(AS_BOUND_METHOD(value)->method->function);
        break;
    case ObjType::CLASS:
        printf("%s", AS_CLASS(value)->name->chars);
        break;
    case ObjType::CLOSURE:
        printFunction(AS_CLOSURE(value)->function);
        break;
    case ObjType::FUNCTION:
        printFunction(AS_FUNCTION(value));
        break;
    case ObjType::INSTANCE:
        printf("%s instance", AS_INSTANCE(value)->klass->name->chars);
        break;
    case ObjType::NATIVE:
        printf("<native fn>");
        break;
    case ObjType::STRING:
        printf("%s", AS_CSTRING(value));
        break;
    case ObjType::UPVALUE:
        printf("upvalue");
        break;
    }
}
