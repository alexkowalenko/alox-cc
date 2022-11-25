# Benchmark programs

include_directories(${PROJECT_SOURCE_DIR}/src)

macro(package_add_benchmark TESTNAME)
        #Add compile target
        add_executable(${TESTNAME} ${ARGN})

        target_link_libraries(${TESTNAME} PRIVATE project_options project_warnings
                                         lox linenoise benchmark::benchmark)
endmacro() 

package_add_benchmark(bench_test bench_test.cc)
package_add_benchmark(bench_file bench_file.cc)
