
# ICU unicode library
set(ICU_INCLUDE_DIRS /opt/homebrew/opt/icu4c/include)
set(ICU_LIBRARY_DIRS /opt/homebrew/opt/icu4c/lib)
set(ICU_LIBRARIES icuuc)
link_directories(${ICU_LIBRARY_DIRS})

message(STATUS "icuuc: [${ICU_LIBRARY_DIRS}]")

include(cmake/CPM.cmake)


CPMAddPackage(
        NAME GoogleTest
        GITHUB_REPOSITORY google/googletest
        GIT_TAG v1.17.0
        OPTIONS
        "INSTALL_GTEST OFF"
        "BUILD_GMOCK OFF"
)
CPMAddPackage(
        NAME benchmark
        GITHUB_REPOSITORY google/benchmark
        GIT_TAG v1.9.5
        OPTIONS
        "BENCHMARK_ENABLE_INSTALL OFF"
        "BENCHMARK_ENABLE_TESTING OFF"
)

CPMAddPackage("gh:AmokHuginnsson/replxx#release-0.0.4")

CPMAddPackage(
        NAME utfcpp
        GITHUB_REPOSITORY alexkowalenko/utfcpp
        GIT_TAG master
        OPTIONS
        "UTFCPP_BUILD_TESTS OFF"
        "UTFCPP_BUILD_DOC OFF"
        "UTFCPP_INSTALL OFF"
)

CPMAddPackage("gh:CLIUtils/CLI11#v2.6.2")

CPMAddPackage(NAME bdwgc
        GITHUB_REPOSITORY bdwgc/bdwgc
        GIT_TAG v8.2.12
        OPTIONS "BUILD_SHARED_LIBS OFF"
        "install_headers OFF"
        "enable_docs OFF"
        "build_cord OFF")