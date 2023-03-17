
macro(gs_add_test test)
add_test(NAME ${test} COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/${test}.py -e ${CMAKE_SOURCE_DIR}/build/vp -l ${CMAKE_SOURCE_DIR}/conf.lua)
set_tests_properties(${test} PROPERTIES TIMEOUT 30)
endmacro()

gs_add_test(Interaction_test)
