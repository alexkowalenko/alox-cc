include_directories(${PROJECT_SOURCE_DIR}/src)

add_executable(alox main.cc)

target_link_libraries(alox
                    PRIVATE project_options project_warnings
                    lox linenoise)
                    
target_include_directories(alox PUBLIC "${linenoise_SOURCE_DIR}/include")
                    

install(TARGETS alox DESTINATION bin)