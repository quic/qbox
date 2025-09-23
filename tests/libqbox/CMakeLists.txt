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

    find_package(Git QUIET)
    if(Git_FOUND)
        set(PATCH_EXEC_CMD ${GIT_EXECUTABLE})
        set(PATCH_ARGS "apply")
    else()
        message(FATAL_ERROR "Couldn't find git executable for keystone patching")
    endif()
    cpmfindpackage(
        NAME
        keystone
        GIT_REPOSITORY
        https://github.com/keystone-engine/keystone.git
        GIT_TAG
        0.9.2
        GIT_SHALLOW
        TRUE
        PATCH_COMMAND
        ${PATCH_EXEC_CMD} ${PATCH_ARGS} "${CMAKE_CURRENT_SOURCE_DIR}/patch-keystone.patch"
        UPDATE_DISCONNECTED
        1
        OPTIONS
        "BUILD_LIBS_ONLY"
    )

function(qbox_add_cpu_test target timeout_s)
    if(NOT ${keystone_FOUND})
        return()
    endif()

    list(APPEND QBOX_TEST_ACCELS "tcg")
    # if ("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "arm64")
    #         list(APPEND QBOX_TEST_ACCELS "hvf")
    # endif()
    if ("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "aarch64")
            list(APPEND QBOX_TEST_ACCELS "kvm")
    endif()

    add_executable(${target} ${ARGN})

    # Apply HVF entitlements on MacOS
    # if(APPLE)
    #     add_custom_command(
    #         TARGET ${target}
    #         POST_BUILD
    #         COMMAND codesign
    #             --entitlements ${libqemu_SOURCE_DIR}/accel/hvf/entitlements.plist
    #             --force -s - $<TARGET_FILE:${target}>
    #         COMMENT "Applying HVF entitlements to $<TARGET_FILE:${target}>"
    #     )
    # endif()

    set(SINGLE_CPU_TGTS "hexagon" "riscv32")


    install(TARGETS ${target} DESTINATION bin)

    target_include_directories(${target} PRIVATE ${PROJECT_SOURCE_DIR}/tests/libqbox/include ${keystone_SOURCE_DIR}/include)

    target_link_libraries(${target} PRIVATE cpu_arm_cortexA53 qemu_cpu_hexagon cpu_riscv32 router reset_gpio gs_memory exclusive_monitor display plic_sifive ${TARGET_LIBS} keystone)

    foreach (accel ${QBOX_TEST_ACCELS})
      if(("${target}" MATCHES "hexagon") AND (NOT ("${accel}" STREQUAL "tcg")))
         continue()
      endif()
      foreach(sync_pol ${QBOX_CPU_TEST_SYNC_POLICY_COMBINATION})
        if("${sync_pol}" STREQUAL "tlm2-icount")
            if (${accel} STREQUAL "tcg")
                set(sync_pol "tlm2")
                set(icount "true")
            else()
                continue()
            endif()
        else()
            set(icount "false")
        endif()

        if("${sync_pol}" STREQUAL "multithread-single")
            if (${accel} STREQUAL "tcg")
                set(sync_pol "multithread")
                set(threading "SINGLE")
            else()
                continue()
            endif()
        else ()
            set(threading "MULTI")
        endif()

        if("${sync_pol}" STREQUAL "tlm2")
            if (NOT ("${target}" STREQUAL "hexagon-reset-test"))
                set(threading "COROUTINE")
            else()
                continue()
            endif()
        endif()


        foreach(num_cpu ${QBOX_CPU_TEST_NUM_CPU_COMBINATION})
            # Skip multi-cpu tests for now
            set(skip_multi TRUE)
            foreach(tgt_name IN LISTS SINGLE_CPU_TGTS)
                if("${tgt_name}" MATCHES ".*${target}.*")
                    set(skip_multi FALSE)
                    break()
                endif()
            endforeach()
            if((${skip_multi}) AND (NOT (${num_cpu} EQUAL 1)))
               continue()
            endif()
            set(test_name ${target}:sync-pol=${sync_pol}:num-cpu=${num_cpu}:icount=${icount}:threading=${threading}:accel=${accel})

            add_test(
                NAME ${test_name}
                COMMAND $<TARGET_FILE:${target}> -p test-bench.inst_a.sync_policy=\"${sync_pol}\"
                                                 -p test-bench.inst_b.sync_policy=\"${sync_pol}\"
                                                 -p test-bench.inst_a.accel=\"${accel}\"
                                                 -p test-bench.inst_b.accel=\"${accel}\"
                                                 -p test-bench.num_cpu=${num_cpu}
                                                 -p test-bench.inst_a.icount=${icount}
                                                 -p test-bench.inst_b.icount=${icount}
                                                 $<$<STREQUAL:${icount},true>:-p test-bench.inst_a.icount_mips_shift=3>
                                                 $<$<STREQUAL:${icount},true>:-p test-bench.inst_b.icount_mips_shift=3>
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
    endforeach()
endfunction(qbox_add_cpu_test)

add_subdirectory(cpu)

set(QBOX_TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR})

function(qbox_extra_add_test target)
    add_executable(${target} ${ARGN})
    install(TARGETS ${target} DESTINATION bin)
    target_include_directories(${target} PRIVATE ${QBOX_TEST_DIR}/include)
    target_link_libraries(${target} PRIVATE ${TARGET_LIBS} display)
    add_test(NAME ${target} COMMAND $<TARGET_FILE:${target}>)
endfunction(qbox_extra_add_test)

add_subdirectory(display)

add_subdirectory(hexagon)
