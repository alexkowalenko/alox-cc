//
// ALOX-CC
//

#pragma once

#include <cstddef>
#include <cstdint>

namespace alox {

#define NAN_BOXING

constexpr auto UINT8_COUNT = UINT8_MAX + 1; // 256

#ifndef UINT8_WIDTH
constexpr auto UINT8_WIDTH = sizeof(uint8_t) * 8;
#endif

using Char = uint32_t;

} // namespace alox