
macro(gs_add_test test)
    add_test(NAME ${test} COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/${test}.py -e ${CMAKE_SOURCE_DIR}/build/platforms/cortex-a53/cortex-a53-vp -l ${CMAKE_SOURCE_DIR}/platforms/cortex-a53/conf.lua)
    set_tests_properties(${test} PROPERTIES TIMEOUT 30)
endmacro()

gs_add_test(cortex_a53)
