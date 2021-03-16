macro(gs_test test)
    add_executable(${test} ${test}.cc)
    target_link_libraries(${test} SystemC::systemc SystemC::cci libgsutils)
    add_test(NAME ${test}_gslog COMMAND ${test})
    set_tests_properties(${test}_gslog PROPERTIES ENVIRONMENT GS_LOG=1)
    set_tests_properties(${test}_gslog PROPERTIES TIMEOUT 10)
    add_test(NAME ${test} COMMAND ${test})
    set_tests_properties(${test} PROPERTIES TIMEOUT 10)
endmacro()

gs_systemc()


gs_test(logger_test)

add_executable(lua_test lua_test.cc)
target_link_libraries(lua_test SystemC::systemc SystemC::cci libgsutils)
add_test(NAME lua_test COMMAND lua_test --gs_luafile ${CMAKE_CURRENT_SOURCE_DIR}/lua_test.lua --param top.cmdvalue=1010 --p top.allvalue=1050)
set_tests_properties(lua_test PROPERTIES TIMEOUT 10)