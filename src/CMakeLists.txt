
include_directories(${PROJECT_SOURCE_DIR}/src)

add_library(lox STATIC
   chunk.cc
   compiler.cc
   debug.cc
   memory.cc
   object.cc
   parser.cc
   scanner.cc
   table.cc
   value.cc
   val_array.cc
   vm.cc
   alox.cc
   )

target_link_libraries(lox fmt)

target_include_directories(lox PUBLIC "${linenoise_SOURCE_DIR}/include")
