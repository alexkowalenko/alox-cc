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
        alox.runFile(extra_args...);
    }
}

BENCHMARK_CAPTURE(BM_Test, binary_trees, "../benchmarks/binary_trees.lox");
BENCHMARK_CAPTURE(BM_Test, equality, "../benchmarks/equality.lox");
BENCHMARK_CAPTURE(BM_Test, fib, "../benchmarks/fib.lox");
BENCHMARK_CAPTURE(BM_Test, instantiation, "../benchmarks/instantiation.lox");
BENCHMARK_CAPTURE(BM_Test, invocation, "../benchmarks/invocation.lox");
BENCHMARK_CAPTURE(BM_Test, method_call, "../benchmarks/method_call.lox");
BENCHMARK_CAPTURE(BM_Test, properties, "../benchmarks/properties.lox");
// BENCHMARK_CAPTURE(BM_Test, string_equality, "../benchmarks/string_equality.lox");
BENCHMARK_CAPTURE(BM_Test, trees, "../benchmarks/trees.lox");
BENCHMARK_CAPTURE(BM_Test, zoo_batch, "../benchmarks/zoo_batch.lox");
BENCHMARK_CAPTURE(BM_Test, zoo, "../benchmarks/zoo.lox");

// Run the benchmark
BENCHMARK_MAIN();