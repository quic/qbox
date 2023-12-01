macro(gs_test test)
    add_executable(${test} ${test}.cc)
    target_link_libraries(${test} ${TARGET_LIBS})
    add_test(NAME ${test}_gslog COMMAND ${test})
    set_tests_properties(${test}_gslog PROPERTIES ENVIRONMENT GS_LOG=1)
    set_tests_properties(${test}_gslog PROPERTIES TIMEOUT 10)
    add_test(NAME ${test} COMMAND ${test})
    set_tests_properties(${test} PROPERTIES TIMEOUT 10)
endmacro()

# gs_systemc()


gs_test(logger_test)

add_executable(lua_test lua_test.cc)
target_link_libraries(lua_test ${TARGET_LIBS})
add_test(NAME lua_test COMMAND lua_test --gs_luafile ${CMAKE_CURRENT_SOURCE_DIR}/lua_test.lua --param top.cmdvalue=1010 --p top.allvalue=1050)
set_tests_properties(lua_test PROPERTIES TIMEOUT 10)

add_executable(cci_test cci_test.cc)
target_link_libraries(cci_test ${TARGET_LIBS})
add_test(NAME cci_test COMMAND cci_test --gs_luafile ${CMAKE_CURRENT_SOURCE_DIR}/lua_test.lua -p MyTopThree.MyCtrl.log_level=0)
set_tests_properties(cci_test PROPERTIES TIMEOUT 10)
if (GS_ENABLE_SANITIZERS)
    set(ENV{ASAN_OPTIONS} "detect_leaks=1")
    set(ENV{LSAN_OPTIONS} "suppressions=${CMAKE_CURRENT_SOURCE_DIR}/ignore_leaks.txt")
    set_tests_properties(cci_test PROPERTIES
        ENVIRONMENT ASAN_OPTIONS="detect_leaks=1":$ENV{ASAN_OPTIONS}
        ENVIRONMENT LSAN_OPTIONS="suppressions=${CMAKE_CURRENT_SOURCE_DIR}/ignore_leaks.txt":$ENV{LSAN_OPTIONS})
endif()

add_executable(cci_alias_test cci_alias_test.cc)
target_link_libraries(cci_alias_test ${TARGET_LIBS})
add_test(NAME cci_alias_test COMMAND cci_alias_test --param MyTopThree.lua_conf=\"${CMAKE_CURRENT_SOURCE_DIR}/lua_test.lua\")
set_tests_properties(cci_alias_test PROPERTIES TIMEOUT 10)
if (GS_ENABLE_SANITIZERS)
    set(ENV{ASAN_OPTIONS} "detect_leaks=1")
    set(ENV{LSAN_OPTIONS} "suppressions=${CMAKE_CURRENT_SOURCE_DIR}/ignore_leaks.txt")
    set_tests_properties(cci_alias_test PROPERTIES
        ENVIRONMENT ASAN_OPTIONS="detect_leaks=1":$ENV{ASAN_OPTIONS}
        ENVIRONMENT LSAN_OPTIONS="suppressions=${CMAKE_CURRENT_SOURCE_DIR}/ignore_leaks.txt":$ENV{LSAN_OPTIONS})
endif()

add_executable(scp_loggging_test scp_loggging_test.cc)
target_link_libraries(scp_loggging_test ${TARGET_LIBS} scp::tlm_extensions::initiator_id scp::tlm_extensions::path_trace)
add_test(NAME scp_loggging_test COMMAND scp_loggging_test --gs_luafile ${CMAKE_CURRENT_SOURCE_DIR}/lua_test.lua --param log_level=0)
set_tests_properties(scp_loggging_test PROPERTIES TIMEOUT 10)
if (GS_ENABLE_SANITIZERS)
    set(ENV{ASAN_OPTIONS} "detect_leaks=1")
    set(ENV{LSAN_OPTIONS} "suppressions=${CMAKE_CURRENT_SOURCE_DIR}/ignore_leaks.txt")
    set_tests_properties(scp_loggging_test PROPERTIES
        ENVIRONMENT ASAN_OPTIONS="detect_leaks=1":$ENV{ASAN_OPTIONS}
        ENVIRONMENT LSAN_OPTIONS="suppressions=${CMAKE_CURRENT_SOURCE_DIR}/ignore_leaks.txt":$ENV{LSAN_OPTIONS})
endif()

add_subdirectory(factory)
