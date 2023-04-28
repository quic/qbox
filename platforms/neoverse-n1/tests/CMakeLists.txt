
macro(gs_add_test test)
    add_test(NAME ${test} COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/${test}.py -e ${CMAKE_SOURCE_DIR}/build/platforms/neoverse-n1/neoverse-n1-vp -l ${CMAKE_SOURCE_DIR}/platforms/neoverse-n1/conf.lua)
    set_tests_properties(${test} PROPERTIES TIMEOUT 30)
endmacro()

gs_add_test(neoverse_n1)
