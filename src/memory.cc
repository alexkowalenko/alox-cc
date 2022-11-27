//
// ALOX-CC
//

#include <stdlib.h>

#include "compiler.hh"
#include "memory.hh"
#include "vm.hh"

#ifdef DEBUG_LOG_GC
#include "debug.h"
#include <stdio.h>
#endif

#define GC_HEAP_GROW_FACTOR 2

GC gc;

void *reallocate(void *pointer, size_t oldSize, size_t newSize) {
    gc.bytesAllocated += newSize - oldSize;
    if (newSize > oldSize) {
        if constexpr (DEBUG_STRESS_GC) {
            gc.collectGarbage();
        }
        if (gc.bytesAllocated > gc.nextGC) {
            gc.collectGarbage();
        }
    }

    if (newSize == 0) {
        free(pointer);
        return nullptr;
    }

    void *result = realloc(pointer, newSize);
    if (result == nullptr) {
        exit(1);
    }
    return result;
}

void GC::markObject(Obj *object) {
    if (object == nullptr) {
        return;
    }
    if (object->isMarked) {
        return;
    }

    if constexpr (DEBUG_LOG_GC) {
        printf("%p mark ", (void *)object);
        printValue(OBJ_VAL(object));
        printf("\n");
    }

    object->isMarked = true;

    if (this->grayCapacity < this->grayCount + 1) {
        this->grayCapacity = grow_capacity(this->grayCapacity);
        this->grayStack =
            (Obj **)realloc(this->grayStack, sizeof(Obj *) * this->grayCapacity);

        if (this->grayStack == nullptr) {
            exit(1);
        }
    }

    this->grayStack[this->grayCount++] = object;
}

void GC::markValue(Value value) {
    if (IS_OBJ(value)) {
        markObject(AS_OBJ(value));
    }
}

void GC::blackenObject(Obj *object) {
    if constexpr (DEBUG_LOG_GC) {
        printf("%p blacken ", (void *)object);
        printValue(OBJ_VAL(object));
        printf("\n");
    }

    switch (object->type) {
    case OBJ_BOUND_METHOD: {
        ObjBoundMethod *bound = (ObjBoundMethod *)object;
        markValue(bound->receiver);
        markObject((Obj *)bound->method);
        break;
    }
    case OBJ_CLASS: {
        ObjClass *klass = (ObjClass *)object;
        markObject((Obj *)klass->name);
        klass->methods.mark();
        break;
    }
    case OBJ_CLOSURE: {
        ObjClosure *closure = (ObjClosure *)object;
        markObject((Obj *)closure->function);
        for (int i = 0; i < closure->upvalueCount; i++) {
            markObject((Obj *)closure->upvalues[i]);
        }
        break;
    }
    case OBJ_FUNCTION: {
        ObjFunction *function = (ObjFunction *)object;
        markObject((Obj *)function->name);
        function->chunk.get_constants().mark();
        break;
    }
    case OBJ_INSTANCE: {
        ObjInstance *instance = (ObjInstance *)object;
        markObject((Obj *)instance->klass);
        instance->fields.mark();
        break;
    }
    case OBJ_UPVALUE:
        markValue(((ObjUpvalue *)object)->closed);
        break;
    case OBJ_NATIVE:
    case OBJ_STRING:
        break;
    }
}

void GC::freeObject(Obj *object) {
    if constexpr (DEBUG_LOG_GC) {
        printf("%p free type %d\n", (void *)object, object->type);
    }

    switch (object->type) {
    case OBJ_BOUND_METHOD:
        FREE<ObjBoundMethod>(object);
        break;
    case OBJ_CLASS: {
        ObjClass *klass = (ObjClass *)object;
        klass->methods.free();
        FREE<ObjClass>(object);
        break;
    } // [braces]
    case OBJ_CLOSURE: {
        ObjClosure *closure = (ObjClosure *)object;
        free_array<ObjUpvalue *>(closure->upvalues, closure->upvalueCount);
        FREE<ObjClosure>(object);
        break;
    }
    case OBJ_FUNCTION: {
        ObjFunction *function = (ObjFunction *)object;
        function->chunk.free();
        FREE<ObjFunction>(object);
        break;
    }
    case OBJ_INSTANCE: {
        ObjInstance *instance = (ObjInstance *)object;
        instance->fields.free();
        FREE<ObjInstance>(object);
        break;
    }
    case OBJ_NATIVE:
        FREE<ObjNative>(object);
        break;
    case OBJ_STRING: {
        ObjString *string = (ObjString *)object;
        free_array<char>(string->chars, string->length + 1);
        FREE<ObjString>(object);
        break;
    }
    case OBJ_UPVALUE:
        FREE<ObjUpvalue>(object);
        break;
    }
}

void GC::markRoots() {
    for (Value *slot = vm.stack; slot < vm.stackTop; slot++) {
        markValue(*slot);
    }

    for (int i = 0; i < vm.frameCount; i++) {
        markObject((Obj *)vm.frames[i].closure);
    }

    for (ObjUpvalue *upvalue = vm.openUpvalues; upvalue != nullptr;
         upvalue = upvalue->next) {
        markObject((Obj *)upvalue);
    }

    vm.globals.mark();
    vm.compiler->markCompilerRoots();
    markObject((Obj *)vm.initString);
}

void GC::traceReferences() {
    while (grayCount > 0) {
        Obj *object = grayStack[--grayCount];
        blackenObject(object);
    }
}

void GC::sweep() {
    Obj *previous = nullptr;
    Obj *object = this->objects;
    while (object != nullptr) {
        if (object->isMarked) {
            object->isMarked = false;
            previous = object;
            object = object->next;
        } else {
            Obj *unreached = object;
            object = object->next;
            if (previous != nullptr) {
                previous->next = object;
            } else {
                this->objects = object;
            }

            freeObject(unreached);
        }
    }
}

void GC::collectGarbage() {
    static size_t before;
    if constexpr (DEBUG_LOG_GC) {
        printf("-- gc begin\n");
        size_t before = this->bytesAllocated;
    }

    markRoots();
    traceReferences();
    vm.strings.removeWhite();
    sweep();

    this->nextGC = this->bytesAllocated * GC_HEAP_GROW_FACTOR;

    if constexpr (DEBUG_LOG_GC) {
        printf("-- gc end\n");
        printf("   collected %zu bytes (from %zu to %zu) next at %zu\n",
               before - this->bytesAllocated, before, this->bytesAllocated, this->nextGC);
    }
}

void GC::freeObjects() {
    Obj *object = this->objects;
    while (object != nullptr) {
        Obj *next = object->next;
        freeObject(object);
        object = next;
    }

    free(this->grayStack);
}

void GC::init() {
    objects = nullptr;
    bytesAllocated = 0;
    nextGC = 1024 * 1024;

    grayCount = 0;
    grayCapacity = 0;
    grayStack = nullptr;
}
