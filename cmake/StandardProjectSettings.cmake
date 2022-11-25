set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set a default build type if none was specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'RelWithDebInfo' as none was specified.")
  set(CMAKE_BUILD_TYPE
      RelWithDebInfo
      CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui, ccmake
  set_property(
    CACHE CMAKE_BUILD_TYPE
    PROPERTY STRINGS
             "Debug"
             "Release"
             "MinSizeRel"
             "RelWithDebInfo")
endif()

# Generate compile_commands.json to make it easier to work with clang based tools
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(ENABLE_IPO "Enable Interprocedural Optimization, aka Link Time Optimization (LTO)" OFF)

if(ENABLE_IPO)
  include(CheckIPOSupported)
  check_ipo_supported(
    RESULT
    result
    OUTPUT
    output)
  if(result)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
  else()
    message(SEND_ERROR "IPO is not supported: ${output}")
  endif()
endif()

# ICU unicode library
# set(ICU_INCLUDE_DIRS /usr/local/opt/icu4c/include)
# set(ICU_LIBRARY_DIRS /usr/local/opt/icu4c/lib)
# set(ICU_LIBRARIES icuuc)
# include_directories(${ICU_INCLUDE_DIRS})
# link_directories(${ICU_LIBRARY_DIRS})

include(cmake/CPM.cmake)
CPMAddPackage("gh:fmtlib/fmt#9.1.0")
CPMAddPackage("gh:arangodb/linenoise-ng#v1.0.1")
# CPMAddPackage("gh:nemtrif/utfcpp#v3.2.1")
# CPMAddPackage("gh:CLIUtils/CLI11#v2.3.0")