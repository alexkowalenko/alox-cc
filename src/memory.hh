//
// ALOX-CC
//

#pragma once

#include "common.hh"
#include "object.hh"
#include "vm.hh"

class GC {
  public:
    // global string table;
    Table strings;

    void init(VM *vm);
    void free();

    void *reallocate(void *pointer, size_t oldSize,
                     size_t newSize);                // Main memory allocation routine.
    Obj  *allocateObject(size_t size, ObjType type); // Allocates all the objects

    template <typename T> T *allocateObject(ObjType type);

    void markObject(Obj *object);
    void markValue(Value value);
    void collectGarbage();
    void freeObjects();

    void trigger(size_t oldSize, size_t newSize);

  private:
    void placeObject(Obj *object, ObjType type);
    void blackenObject(Obj *object);
    void freeObject(Obj *object);
    void markRoots();
    void traceReferences();
    void sweep();

    VM *vm{nullptr};

    size_t bytesAllocated;
    size_t nextGC;
    Obj   *objects;
    int    grayCount;
    int    grayCapacity;
    Obj  **grayStack;
};

extern GC gc;

template <typename T> T *GC::allocateObject(ObjType type) {
    trigger(0, sizeof(T));
    auto *object = new T;
    placeObject((Obj *)object, type);
    if constexpr (DEBUG_LOG_GC) {
        printf("%p allocate %zu for %d\n", (void *)object, sizeof(T), type);
    }
    return object;
}

template <typename T> constexpr T *allocate(size_t count) {
    return (T *)gc.reallocate(nullptr, 0, sizeof(T) * (count));
}

template <typename T> constexpr void FREE(Obj *pointer) {
    gc.reallocate(pointer, sizeof(T), 0);
}

template <typename T> constexpr void del(T *pointer) {
    delete pointer;
    gc.trigger(sizeof(T), 0);
}

constexpr size_t grow_capacity(size_t capacity) {
    return ((capacity) < 8 ? 8 : (capacity)*2);
};

template <typename T>
constexpr T *grow_array(T *pointer, size_t oldCount, size_t newCount) {
    return (T *)gc.reallocate(pointer, sizeof(T) * (oldCount), sizeof(T) * (newCount));
}

template <typename T> constexpr void free_array(T *pointer, size_t oldCount) {
    gc.reallocate(pointer, sizeof(T) * (oldCount), 0);
}
