//
// ALOX-CC
//

#pragma once

#include <cstddef>
#include <cstdint>

#define NAN_BOXING
#define DEBUG_PRINT_CODE
#define DEBUG_TRACE_EXECUTION

#define DEBUG_STRESS_GC
#define DEBUG_LOG_GC

constexpr auto UINT8_COUNT = UINT8_MAX + 1;

// In the book, we show them defined, but for working on them locally,
// we don't want them to be.
#undef DEBUG_PRINT_CODE
#undef DEBUG_TRACE_EXECUTION
#undef DEBUG_STRESS_GC
#undef DEBUG_LOG_GC
