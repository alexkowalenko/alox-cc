//
// ALOX-CC
//

#include <benchmark/benchmark.h>

#include "alox.hh"

template <typename... ExtraArgs>
static void BM_Test(benchmark::State &state, ExtraArgs &&...extra_args) {
    // Perform setup here

    Alox alox;

    for (auto _ : state) {
        // This code gets timed
        alox.runString(extra_args...);
    }
}

BENCHMARK_CAPTURE(BM_Test, print, "print 1;");
BENCHMARK_CAPTURE(BM_Test, function, "fun f(x) {return x;} var y = f(1);");

// Run the benchmark
BENCHMARK_MAIN();