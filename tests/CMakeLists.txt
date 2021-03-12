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
