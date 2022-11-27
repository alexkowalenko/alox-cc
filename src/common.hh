//
// ALOX-CC
//

#pragma once

#include <cstddef>
#include <cstdint>

#define NAN_BOXING

constexpr auto DEBUG_TRACE_EXECUTION{false};
constexpr auto DEBUG_STRESS_GC{false};
constexpr auto DEBUG_LOG_GC{false};

constexpr auto UINT8_COUNT = UINT8_MAX + 1;

constexpr auto MAX_ARGS = UINT8_MAX;
