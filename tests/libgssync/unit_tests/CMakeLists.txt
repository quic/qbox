
macro(gs_test test)
    add_executable(${test} ${test}.cc)
    target_link_libraries(${test} ${TARGET_LIBS} gtest gmock)
    add_test(NAME ${test} COMMAND ${test})
    set_tests_properties(${test} PROPERTIES TIMEOUT 10)
endmacro()

gs_test(qk_extendedif_test)
gs_test(qkmultithread_test)
gs_test(qkmulti-quantum_test)
