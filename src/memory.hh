//
// ALOX-CC
//

#pragma once

#include "common.hh"
#include "object.hh"
#include "vm.hh"
#include <concepts>

namespace alox {

/**
 * @brief new allocator based on new/delete
 *
 * @tparam T
 * @param type Object type
 * @return T*
 */
template <typename T>
    requires std::default_initializable<T>
T *allocateObject(ObjType /*type*/) {
    auto *object = new T();
    return object;
}

/**
 * @brief new deallocator based on new/delete
 *
 * @tparam T
 * @param pointer
 */
template <typename T>
    requires std::destructible<T>
constexpr void deleteObject(T *pointer) {
    delete pointer;
}

constexpr size_t grow_capacity(size_t capacity) {
    return ((capacity) < 8 ? 8 : (capacity)*2);
};

template <typename T>
    requires std::default_initializable<T> && std::destructible<T>
constexpr T *grow_array(T *pointer, size_t oldCount, size_t newCount) {
    auto result = new T[newCount];
    memcpy(result, pointer, sizeof(T) * oldCount);
    delete[] pointer;
    return result;
};

} // namespace alox