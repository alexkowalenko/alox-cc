cmake_minimum_required(VERSION 3.24)

# Change compiler here
set(AX_BASE_CLANG /usr/local/opt/llvm)
set(CMAKE_CXX_COMPILER ${AX_BASE_CLANG}/bin/clang++)
message(STATUS "C++ compiler = ${CMAKE_CXX_COMPILER}")

set(CMAKE_BUILD_TYPE Release)
# set(CMAKE_BUILD_TYPE DEBUG)

# Set the project name to your project name, my project isn't very descriptive
project(alox-cc CXX)
include(cmake/StandardProjectSettings.cmake)
include(cmake/PreventInSourceBuilds.cmake)


# Link this 'library' to set the c++ standard / compile-time options requested
add_library(project_options INTERFACE)
target_compile_features(project_options INTERFACE cxx_std_23)

if(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
  option(ENABLE_BUILD_WITH_TIME_TRACE "Enable -ftime-trace to generate time tracing .json files on clang" OFF)
  if(ENABLE_BUILD_WITH_TIME_TRACE)
    add_compile_definitions(project_options INTERFACE -ftime-trace)
  endif()
endif()

# Link this 'library' to use the warnings specified in CompilerWarnings.cmake
add_library(project_warnings INTERFACE)

# enable cache system
include(cmake/Cache.cmake)

# standard compiler warnings
include(cmake/CompilerWarnings.cmake)
set_project_warnings(project_warnings)

# sanitizer options if supported by compiler
include(cmake/Sanitizers.cmake)
enable_sanitizers(project_options)

# enable doxygen
include(cmake/Doxygen.cmake)
enable_doxygen()

# allow for static analysis options
include(cmake/StaticAnalyzers.cmake)

option(BUILD_SHARED_LIBS "Enable compilation of shared libraries" OFF)
option(ENABLE_TESTING "Enable Test Builds" ON)
option(ENABLE_BENCHMARKS "Enable Benchmarks Builds" OFF)
option(ENABLE_FUZZING "Enable Fuzzing Builds" OFF)

if(ENABLE_TESTING)

  CPMAddPackage(
    NAME googletest
    GITHUB_REPOSITORY google/googletest
    GIT_TAG release-1.12.1
    VERSION 1.12.1
    OPTIONS
      "INSTALL_GTEST OFF"
      "BUILD_GMOCK OFF"
      "gtest_force_shared_crt ON"
      "gtest_disable_pthreads ON" 
      "gtest_hide_internal_symbols ON"
  )

  enable_testing()
  add_subdirectory(test)
endif()

if(ENABLE_FUZZING)
  message("Building Fuzz Tests, using fuzzing sanitizer https://www.llvm.org/docs/LibFuzzer.html")
  add_subdirectory(fuzz_test)
endif()

if(ENABLE_BENCHMARKS)
  CPMAddPackage(
    NAME benchmark
    GITHUB_REPOSITORY google/benchmark
    GIT_TAG v1.7.1
    OPTIONS
      "BENCHMARK_ENABLE_INSTALL OFF"
      "BENCHMARK_ENABLE_TESTING OFF"
  )
  message("Building Benchmarks")
  add_subdirectory(benchmarks)
endif()

add_subdirectory(src)
add_subdirectory(cmd)

# install
set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR})

