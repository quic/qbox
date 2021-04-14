find_package(PkgConfig)
pkg_check_modules(keystone IMPORTED_TARGET keystone)

list(APPEND QBOX_CPU_TEST_SYNC_POLICY_COMBINATION
    tlm2
    tlm2-icount
    multithread
    multithread-quantum)

list(APPEND QBOX_CPU_TEST_NUM_CPU_COMBINATION 1 2 4 8 16 32 64 128)

function(qbox_add_cpu_test target timeout_s)
    add_executable(${target} ${ARGN})

    target_include_directories(${target}
        PRIVATE $<TARGET_PROPERTY:qbox,INCLUDE_DIRECTORIES>
        PRIVATE ${CMAKE_SOURCE_DIR}/tests/include)

    target_link_libraries(${target} qbox PkgConfig::keystone)

    foreach(sync_pol ${QBOX_CPU_TEST_SYNC_POLICY_COMBINATION})
        if ("${sync_pol}" STREQUAL "tlm2-icount")
            set(sync_pol tlm2)
            set(icount "true")
        else()
            set(icount "false")
        endif()

        foreach(num_cpu ${QBOX_CPU_TEST_NUM_CPU_COMBINATION})
            set(test_name ${target}|sync-pol:${sync_pol}|num-cpu:${num_cpu}|icount:${icount})

            add_test(
                NAME ${test_name}
                COMMAND $<TARGET_FILE:${target}> -p test-bench.sync-policy=\"${sync_pol}\"
                                                 -p test-bench.num-cpu=${num_cpu}
                                                 -p test-bench.icount=${icount})
            set_property(TEST ${test_name} PROPERTY TIMEOUT ${timeout_s})
        endforeach()
    endforeach()
endfunction(qbox_add_cpu_test)

add_subdirectory(cpu)
