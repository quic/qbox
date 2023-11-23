CPMAddPackage("gh:google/googletest#master")
macro(gs_add_test test)
    add_executable(${test} ${test}.cc)
    target_include_directories(${test}
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/greensocs/systemc-uarts/
    )
    target_link_libraries(${test} PRIVATE gtest gmock ${TARGET_LIBS})
    add_test(NAME ${test} COMMAND ${test})
    set_tests_properties(${test} PROPERTIES TIMEOUT 10)
endmacro()
gs_add_test(file-backend-test)
gs_add_test(uart-biflow-stdio-test)
gs_add_test(uart-biflow-backend-socket-test)
gs_add_test(uart-ibex-biflow-stdio-test)
