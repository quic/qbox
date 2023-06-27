CPMAddPackage("gh:google/googletest#main")
macro(gs_add_test test)
    add_executable(${test} ${test}.cc)
    target_include_directories(${test}
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include/
        PUBLIC ${CMAKE_BINARY_DIR}/gen
    )
    target_link_libraries(${test} PRIVATE ${TARGET_LIBS} libzip::zip)
    add_compile_definitions( PROJECT_SOURCE=${PROJECT_SOURCE_DIR} )
# the 'DEPENDS does not work 
    add_test(NAME ${test} COMMAND ${test} -p top.lua_conf=\"${CMAKE_BINARY_DIR}/gen/gen.lua\" DEPENDS ${PROJECT_SOURCE_DIR}/json.zip)
    set_tests_properties(${test} PROPERTIES TIMEOUT 100)
endmacro()
gs_add_test(gen-test)
