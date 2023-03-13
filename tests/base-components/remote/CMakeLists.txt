CPMAddPackage("gh:google/googletest#master")
macro(gs_add_test test)
    add_executable(${test} ${test}.cc)
    target_include_directories(${test}
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../include/greensocs/base-components/
    )
    target_link_libraries(${test} PRIVATE gtest gmock base-components ${TARGET_LIBS})
    add_test(NAME ${test} COMMAND ${test})
    set_tests_properties(${test} PROPERTIES TIMEOUT 10)
endmacro()

add_executable(remote-tests-remote remote.cc)
target_include_directories(remote-tests-remote
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../include/greensocs/base-components/
)
target_link_libraries(remote-tests-remote PRIVATE base-components ${TARGET_LIBS})

gs_add_test(remote-tests)