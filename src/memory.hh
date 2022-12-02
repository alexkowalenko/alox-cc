//
// ALOX-CC
//

#pragma once

#include "common.hh"
#include "object.hh"
#include "vm.hh"

inline constexpr auto Base_GC_Size = 1024 * 1024;

class GC {
  public:
    GC() = default;
    ~GC() = default;

    void init(VM *vm);

    // New allocators
    template <typename T> T             *allocateObject(ObjType type);
    template <typename T> T             *allocate_array(size_t count);
    template <typename T> constexpr void deleteObject(T *pointer);
    template <typename T> constexpr void delete_array(T *pointer, size_t oldCount);

    template <typename T>
    constexpr T *grow_array(T *pointer, size_t oldCount, size_t newCount) {
        auto result = allocate_array<T>(newCount);
        memcpy(result, pointer, sizeof(T) * oldCount);
        delete_array<T>(pointer, oldCount);
        return result;
    };

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

    size_t bytesAllocated{0};
    size_t nextGC{Base_GC_Size};
    Obj   *objects{nullptr};
    size_t grayCount{0};
    size_t grayCapacity{0};
    Obj  **grayStack{nullptr};
};

extern GC gc;

/**
 * @brief new allocator based on new/delete
 *
 * @tparam T
 * @param type Object type
 * @return T*
 */
template <typename T> T *GC::allocateObject(ObjType type) {
    trigger(0, sizeof(T));
    auto *object = new T;
    placeObject((Obj *)object, type);
    if constexpr (DEBUG_LOG_GC) {
        printf("%p allocate %zu for %d\n", (void *)object, sizeof(T), type);
    }
    return object;
}

/**
 * @brief new allocator of arrays based on new/delete
 *
 * @tparam T
 * @param count
 * @return T*
 */
template <typename T> T *GC::allocate_array(size_t count) {
    trigger(0, sizeof(T) * count);
    auto *object = new T[count];
    if constexpr (DEBUG_LOG_GC) {
        printf("%p allocate array %zu (%zu * %zu)\n", (void *)object, sizeof(T) * count,
               sizeof(T), count);
    }
    return object;
}

/**
 * @brief new deallocator based on new/delete
 *
 * @tparam T
 * @param pointer
 */
template <typename T> constexpr void GC::deleteObject(T *pointer) {
    delete pointer;
    trigger(sizeof(T), 0);
}

/**
 * @brief new dellacator of arrays based on new/delete
 *
 * @tparam T
 * @param pointer
 * @param oldCount
 */
template <typename T> constexpr void GC::delete_array(T *pointer, size_t oldCount) {
    delete[] pointer;
    trigger(sizeof(T) * oldCount, 0);
}

constexpr size_t grow_capacity(size_t capacity) {
    return ((capacity) < 8 ? 8 : (capacity)*2);
};