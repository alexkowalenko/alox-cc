

include(GoogleTest)

set_target_properties(gtest PROPERTIES FOLDER _deps)
set_target_properties(gtest_main PROPERTIES FOLDER _deps)

link_directories(${ICU_LIBRARY_DIRS})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

macro(package_add_test TESTNAME)
    add_executable(${TESTNAME} ${ARGN})
    
    # link the Google test infrastructure, mocking library, and a default main fuction to
    # the test executable.  Remove g_test_main if writing your own main function.
    target_link_libraries(${TESTNAME} PRIVATE project_options project_warnings
    lox
    gtest gtest_main fmt icuuc
    )
    
    # gtest_discover_tests replaces gtest_add_tests,
    gtest_discover_tests(${TESTNAME}
        # set a working directory so your project root so that you can find test data via paths relative to the project root
        WORKING_DIRECTORY ${PROJECT_DIR}
        PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_DIR}"
    )
    set_target_properties(${TESTNAME} PROPERTIES FOLDER tests)
endmacro()              

include_directories(${PROJECT_SOURCE_DIR}/src)

package_add_test(scanner.test scanner.test.cc)
package_add_test(string.test string.test.cc)

