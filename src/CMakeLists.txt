
include_directories(${PROJECT_SOURCE_DIR}/src)

add_library(lox STATIC
   options.cc
   chunk.cc
   codegen.cc
   compiler.cc
   context.cc
   debug.cc
   object.cc
   parser.cc
   scanner.cc
   table.cc
   value.cc
   val_array.cc
   vm.cc
   vm_stdlib.cc
   printer.cc
   error.cc
   ast_base.cc
   alox.cc
   )

target_link_libraries(lox fmt replxx)

target_include_directories(lox PUBLIC "${CLI11_SOURCE_DIR}/include")
target_include_directories(lox PUBLIC "${utfcpp_SOURCE_DIR}/source")
target_include_directories(lox PUBLIC "${ICU_INCLUDE_DIRS}")
target_include_directories(lox PUBLIC "${bdwgc_SOURCE_DIR}/include")
