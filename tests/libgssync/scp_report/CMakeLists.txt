macro(gs_add_test test)
    add_executable(${test} ${test}.cc)
    target_include_directories(${test}
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/greensocs/
    )
    target_link_libraries(${test} ${TARGET_LIBS} gtest gmock)
    add_test(NAME ${test} COMMAND ${test} --gs_luafile ${CMAKE_CURRENT_SOURCE_DIR}/lua_test.lua)
    set_tests_properties(${test} PROPERTIES TIMEOUT 10)
endmacro()

gs_add_test(scp_report_thread)
