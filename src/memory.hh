//
// ALOX-CC
//

#pragma once

#include "common.hh"
#include "object.hh"
#include "vm.hh"
#include <concepts>

namespace alox {

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