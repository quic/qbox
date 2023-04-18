SET(LIBQEMU_TARGETS "riscv64")

macro(gs_add_test test lua_file)
    add_executable(${test} ${test}.cc)
    target_include_directories(${test}
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/greensocs/
    )
    target_link_libraries(${test} PRIVATE ${TARGET_LIBS} gtest gmock)
    add_test(NAME ${test} COMMAND ${test} --gs_luafile ${CMAKE_CURRENT_SOURCE_DIR}/${lua_file}.lua)
    set_tests_properties(${test} PROPERTIES TIMEOUT 10) 
endmacro(gs_add_test test lua_file)

gs_add_test(factory_platform factory_platform)