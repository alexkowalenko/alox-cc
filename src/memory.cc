//
// ALOX-CC
//

#include <stdlib.h>

#include "compiler.hh"
#include "memory.hh"
#include "vm.hh"

#define GC_HEAP_GROW_FACTOR 2

GC gc;

inline constexpr auto Base_GC_Size = 1024 * 1024;

void GC::trigger(size_t oldSize, size_t newSize) {
    this->bytesAllocated += newSize - oldSize;
    if (newSize > oldSize) {
        if constexpr (DEBUG_STRESS_GC) {
            this->collectGarbage();
        }
        if (this->bytesAllocated > this->nextGC) {
            this->collectGarbage();
        }
    }
}

void *GC::reallocate(void *pointer, size_t oldSize, size_t newSize) {
    trigger(oldSize, newSize);
    if (newSize == 0) {
        ::free(pointer);
        return nullptr;
    }

    void *result = realloc(pointer, newSize);
    if (result == nullptr) {
        exit(1);
    }
    return result;
}

void GC::placeObject(Obj *object, ObjType type) {
    object->type = type;
    object->isMarked = false;

    object->next = this->objects;
    this->objects = object;
}

Obj *GC::allocateObject(size_t size, ObjType type) {
    Obj *object = (Obj *)this->reallocate(nullptr, 0, size);
    placeObject(object, type);

    if constexpr (DEBUG_LOG_GC) {
        printf("%p allocate %zu for %d\n", (void *)object, size, type);
    }
    return object;
}

void GC::init(VM *vm) {
    objects = nullptr;
    bytesAllocated = 0;
    nextGC = Base_GC_Size;

    grayCount = 0;
    grayCapacity = 0;
    grayStack = nullptr;
    this->vm = vm;
    strings.init();
}

void GC::free() {
    strings.free();
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
    case ObjType::BOUND_METHOD: {
        ObjBoundMethod *bound = (ObjBoundMethod *)object;
        markValue(bound->receiver);
        markObject((Obj *)bound->method);
        break;
    }
    case ObjType::CLASS: {
        ObjClass *klass = (ObjClass *)object;
        markObject((Obj *)klass->name);
        klass->methods.mark();
        break;
    }
    case ObjType::CLOSURE: {
        ObjClosure *closure = (ObjClosure *)object;
        markObject((Obj *)closure->function);
        for (int i = 0; i < closure->upvalueCount; i++) {
            markObject((Obj *)closure->upvalues[i]);
        }
        break;
    }
    case ObjType::FUNCTION: {
        ObjFunction *function = (ObjFunction *)object;
        markObject((Obj *)function->name);
        function->chunk.get_constants().mark();
        break;
    }
    case ObjType::INSTANCE: {
        ObjInstance *instance = (ObjInstance *)object;
        markObject((Obj *)instance->klass);
        instance->fields.mark();
        break;
    }
    case ObjType::UPVALUE:
        markValue(((ObjUpvalue *)object)->closed);
        break;
    case ObjType::NATIVE:
    case ObjType::STRING:
        break;
    }
}

void GC::freeObject(Obj *object) {
    if constexpr (DEBUG_LOG_GC) {
        printf("%p free type %d\n", (void *)object, object->type);
    }

    switch (object->type) {
    case ObjType::BOUND_METHOD:
        del<ObjBoundMethod>((ObjBoundMethod *)object);
        break;
    case ObjType::CLASS: {
        ObjClass *klass = (ObjClass *)object;
        klass->methods.free();
        del(klass);
        // FREE<ObjClass>(object);
        break;
    } // [braces]
    case ObjType::CLOSURE: {
        ObjClosure *closure = (ObjClosure *)object;
        free_array<ObjUpvalue *>(closure->upvalues, closure->upvalueCount);
        del<ObjClosure>(closure);
        break;
    }
    case ObjType::FUNCTION: {
        ObjFunction *function = (ObjFunction *)object;
        function->chunk.free();
        del<ObjFunction>(function);
        break;
    }
    case ObjType::INSTANCE: {
        ObjInstance *instance = (ObjInstance *)object;
        instance->fields.free();
        del<ObjInstance>(instance);
        break;
    }
    case ObjType::NATIVE:
        del<ObjNative>((ObjNative *)object);
        break;
    case ObjType::STRING: {
        ObjString *string = (ObjString *)object;
        free_array<char>(string->chars, string->length + 1);
        FREE<ObjString>(object);
        break;
    }
    case ObjType::UPVALUE:
        del<ObjUpvalue>((ObjUpvalue *)object);
        break;
    }
}

// dependent on vm
void GC::markRoots() {
    vm->markRoots();
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
    this->strings.removeWhite();
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

    ::free(this->grayStack);
}
