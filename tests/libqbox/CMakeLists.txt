list(APPEND QBOX_CPU_TEST_SYNC_POLICY_COMBINATION
    tlm2
    tlm2-icount
    multithread
    multithread-quantum
    multithread-unconstrained
    multithread-single
    )

list(APPEND QBOX_CPU_TEST_NUM_CPU_COMBINATION
    1
    2
    4
    # 8
    # 16
    32
    # 64
    # 128
    )

cpmfindpackage(
    NAME
    keystone
    GIT_REPOSITORY
    https://github.com/keystone-engine/keystone.git
    GIT_TAG
    0.9.2
    GIT_SHALLOW
    TRUE
    OPTIONS
    "BUILD_LIBS_ONLY"
)

function(qbox_add_cpu_test target timeout_s)
    if(NOT ${keystone_FOUND})
        return()
    endif()

    add_executable(${target} ${ARGN})
    install(TARGETS ${target} DESTINATION bin)

    target_include_directories(${target} PRIVATE ${CMAKE_SOURCE_DIR}/tests/libqbox/include ${keystone_SOURCE_DIR}/include)

    target_link_libraries(${target} PRIVATE ${TARGET_LIBS} keystone)

    foreach(sync_pol ${QBOX_CPU_TEST_SYNC_POLICY_COMBINATION})
        if("${sync_pol}" STREQUAL "tlm2-icount")
            set(sync_pol "tlm2")
            set(icount "true")
        else()
            set(icount "false")
        endif()

        if("${sync_pol}" STREQUAL "multithread-single")
           set(sync_pol "multithread")
           set(threading "SINGLE")
        else ()
            set(threading "MULTI")
        endif()

        if("${sync_pol}" STREQUAL "tlm2")
            set(threading "COROUTINE")
        endif()


        foreach(num_cpu ${QBOX_CPU_TEST_NUM_CPU_COMBINATION})
            set(test_name ${target}:sync-pol=${sync_pol}:num-cpu=${num_cpu}:icount=${icount}:threading=${threading})

            add_test(
                NAME ${test_name}
                COMMAND $<TARGET_FILE:${target}> -p test-bench.inst_a.sync_policy=\"${sync_pol}\"
                                                 -p test-bench.inst_b.sync_policy=\"${sync_pol}\"
                                                 -p test-bench.num_cpu=${num_cpu}
                                                 -p test-bench.inst_a.icount=${icount}
                                                 -p test-bench.inst_b.icount=${icount}
                                                 -p test-bench.inst_a.tcg_mode=\"${threading}\"
                                                 -p test-bench.inst_b.tcg_mode=\"${threading}\"

                                                 -p test-bench.bulkmem.max_block_size=1024
                                                 -p test-bench.bulkmem.min_block_size=32

                                                 -p log_level=0
                                                 )
            set_tests_properties(${test_name} PROPERTIES TIMEOUT ${timeout_s})
            if (GS_ENABLE_SANITIZERS)
                # Please see this issue https://bugzilla.redhat.com/show_bug.cgi?id=1449604 
                set_tests_properties(${test_name} PROPERTIES 
                    ENVIRONMENT LD_LIBRARY_PATH="$ENV{LD_LIBRARY_PATH}":$ENV{LD_LIBRARY_PATH})
            endif()
            # Alternative solution until the problem is solved with the exclusive access tests
            # --------------------------------------------------------------------------------
            if(("${target}" STREQUAL "aarch64-ld-st-excl-fail-test") AND ( ("${sync_pol}" STREQUAL "tlm2") OR ("${threading}" STREQUAL "SINGLE") ) )
                set_tests_properties(${test_name} PROPERTIES WILL_FAIL TRUE)
            endif()
            # --------------------------------------------------------------------------------
        endforeach()
    endforeach()
endfunction(qbox_add_cpu_test)

add_subdirectory(cpu)

set(QBOX_TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR})

function(qbox_extra_add_test target)
    add_executable(${target} ${ARGN})
    install(TARGETS ${target} DESTINATION bin)
    target_include_directories(${target} PRIVATE ${QBOX_TEST_DIR}/include)
    target_link_libraries(${target} PRIVATE ${TARGET_LIBS})
    add_test(NAME ${target} COMMAND $<TARGET_FILE:${target}>)
endfunction(qbox_extra_add_test)

add_subdirectory(display)
