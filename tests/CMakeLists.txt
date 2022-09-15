CPMAddPackage("gh:google/googletest#master")
macro(gs_add_test test)
    add_executable(${test} ${test}.cc)
    target_include_directories(${test}
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include/
    )
    target_link_libraries(${test} PRIVATE gtest gmock qup libgsutils scp::reporting
										libgssync)
    add_test(NAME ${test} COMMAND ${test})
    set_tests_properties(${test} PROPERTIES TIMEOUT 10)
endmacro()
gs_add_test(ms-spi-test)

