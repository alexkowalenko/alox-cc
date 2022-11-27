//
// ALOX-CC
//

#pragma once

#include "common.hh"
#include "object.hh"
#include "vm.hh"

class GC {
  public:
    size_t bytesAllocated;
    size_t nextGC;
    Obj   *objects;
    int    grayCount;
    int    grayCapacity;
    Obj  **grayStack;

    // global string table;
    Table strings;

    void init(VM *vm);
    void free();

    void *reallocate(void *pointer, size_t oldSize,
                     size_t newSize);                // Main memory allocation routine.
    Obj  *allocateObject(size_t size, ObjType type); // Allocates all the objects

    void markObject(Obj *object);
    void markValue(Value value);
    void collectGarbage();
    void freeObjects();

  private:
    void blackenObject(Obj *object);
    void freeObject(Obj *object);
    void markRoots();
    void traceReferences();
    void sweep();

    VM *vm{nullptr};
};

extern GC gc;

template <typename T> constexpr T *allocate(size_t count) {
    return (T *)gc.reallocate(nullptr, 0, sizeof(T) * (count));
}

template <typename T> constexpr void FREE(Obj *pointer) {
    gc.reallocate(pointer, sizeof(T), 0);
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
