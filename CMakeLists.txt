cmake_minimum_required(VERSION 4.0)

# Change compiler here
set(AX_BASE_CLANG /opt/homebrew/opt/llvm)
set(CMAKE_CXX_COMPILER ${AX_BASE_CLANG}/bin/clang++)
message(STATUS "C++ compiler = ${CMAKE_CXX_COMPILER}")

# Set the project name to your project name, my project isn't very descriptive
project(alox-cc CXX)
include(cmake/StandardProjectSettings.cmake)
include(cmake/PreventInSourceBuilds.cmake)

add_library(project_options INTERFACE)
target_compile_features(project_options INTERFACE cxx_std_23)

# Dependency management
include(Dependencies.cmake)

if(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
  option(ENABLE_BUILD_WITH_TIME_TRACE "Enable -ftime-trace to generate time tracing .json files on clang" OFF)
  if(ENABLE_BUILD_WITH_TIME_TRACE)
    add_compile_definitions(project_options INTERFACE -ftime-trace)
  endif()
endif()

# Ensure libc++ is used consistently when using Homebrew LLVM on macOS
if (APPLE)
    message(STATUS "Configuring macOS libc++ and runtime flags for Homebrew LLVM")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -L${AX_BASE_CLANG}/lib/c++ -L${AX_BASE_CLANG}/lib/unwind -lunwind")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -stdlib=libc++")
    # Also ensure 64-bit arch is explicit
    set(CMAKE_OSX_ARCHITECTURES arm64)
endif ()

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

#option(BUILD_SHARED_LIBS "Enable compilation of shared libraries" OFF)
set(BUILD_SHARED_LIBS OFF)
option(ENABLE_TESTING "Enable Test Builds" ON)
option(ENABLE_BENCHMARKS "Enable Benchmarks Builds" OFF)
option(ENABLE_FUZZING "Enable Fuzzing Builds" OFF)

if(ENABLE_TESTING)
  enable_testing()
  add_subdirectory(test)
endif()

if(ENABLE_FUZZING)
  message("Building Fuzz Tests, using fuzzing sanitizer https://www.llvm.org/docs/LibFuzzer.html")
  add_subdirectory(fuzz_test)
endif()

if(ENABLE_BENCHMARKS)
  message("Building Benchmarks")
  add_subdirectory(benchmarks)
endif()

add_subdirectory(src)
add_subdirectory(cmd)

# install
set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR})

