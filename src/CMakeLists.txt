
include_directories(${PROJECT_SOURCE_DIR}/src)

add_library(lox STATIC
   chunk.c
   compiler.c
   debug.c
   memory.c
   object.c
   scanner.c
   table.c
   value.c
   vm.c
   )
   
add_executable(alox main.c)

target_link_libraries(alox
                    PRIVATE project_options project_warnings
                    lox)
   
install(TARGETS alox DESTINATION bin)