//
// ALOX-CC
//

#include <iostream>

#include <fmt/core.h>

#include "compiler.hh"
#include "memory.hh"
#include "vm.hh"

inline constexpr auto GC_HEAP_GROW_FACTOR = 2;

GC gc;

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

void GC::placeObject(Obj *object, ObjType type) {
    object->type = type;
    object->isMarked = false;

    object->next = this->objects;
    this->objects = object;
}

void GC::init(VM *vm) {
    this->vm = vm;
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
        fmt::print("{:#0d} mark ", reinterpret_cast<uint64_t>(object));
        printValue(OBJ_VAL(object));
        std::cout << "\n";
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
        fmt::print("{:#0d} blacken ", reinterpret_cast<uint64_t>(object));
        printValue(OBJ_VAL(object));
        std::cout << "\n";
    }

    switch (object->type) {
    case ObjType::BOUND_METHOD: {
        auto *bound = (ObjBoundMethod *)object;
        markValue(bound->receiver);
        markObject((Obj *)bound->method);
        break;
    }
    case ObjType::CLASS: {
        auto *klass = (ObjClass *)object;
        markObject((Obj *)klass->name);
        klass->methods.mark();
        break;
    }
    case ObjType::CLOSURE: {
        auto *closure = (ObjClosure *)object;
        markObject((Obj *)closure->function);
        for (int i = 0; i < closure->upvalueCount; i++) {
            markObject((Obj *)closure->upvalues[i]);
        }
        break;
    }
    case ObjType::FUNCTION: {
        auto *function = (ObjFunction *)object;
        markObject((Obj *)function->name);
        function->chunk.get_constants().mark();
        break;
    }
    case ObjType::INSTANCE: {
        auto *instance = (ObjInstance *)object;
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
        fmt::print("{:#0d} free type {}\n", reinterpret_cast<uint64_t>(object),
                   static_cast<int>(object->type));
    }

    switch (object->type) {
    case ObjType::BOUND_METHOD:
        deleteObject<ObjBoundMethod>((ObjBoundMethod *)object);
        break;
    case ObjType::CLASS: {
        auto *klass = (ObjClass *)object;
        klass->methods.free();
        deleteObject(klass);
        // FREE<ObjClass>(object);
        break;
    } // [braces]
    case ObjType::CLOSURE: {
        auto *closure = (ObjClosure *)object;
        gc.delete_array<ObjUpvalue *>(closure->upvalues, closure->upvalueCount);
        deleteObject<ObjClosure>(closure);
        break;
    }
    case ObjType::FUNCTION: {
        auto *function = (ObjFunction *)object;
        function->chunk.free();
        deleteObject<ObjFunction>(function);
        break;
    }
    case ObjType::INSTANCE: {
        auto *instance = (ObjInstance *)object;
        instance->fields.free();
        deleteObject<ObjInstance>(instance);
        break;
    }
    case ObjType::NATIVE:
        deleteObject<ObjNative>((ObjNative *)object);
        break;
    case ObjType::STRING: {
        auto *string = (ObjString *)object;
        gc.delete_array<char>(string->chars, string->length + 1);
        deleteObject<ObjString>(string);
        break;
    }
    case ObjType::UPVALUE:
        deleteObject<ObjUpvalue>((ObjUpvalue *)object);
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
        std::cout << "-- gc begin\n";
        const size_t before = this->bytesAllocated;
    }

    markRoots();
    traceReferences();
    this->strings.removeWhite();
    sweep();

    this->nextGC = this->bytesAllocated * GC_HEAP_GROW_FACTOR;

    if constexpr (DEBUG_LOG_GC) {
        std::cout << "-- gc end\n";
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
