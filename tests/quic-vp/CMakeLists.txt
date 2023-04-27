
set(QQVP_FIRMWARE_REPO_DIR "$ENV{QQVP_FIRMWARE_REPO_DIR}" CACHE STRING "")

macro(gs_add_test test conf_lua image_dir extra)
    set(test_dir "${QQVP_FIRMWARE_REPO_DIR}/${image_dir}")

    if(EXISTS ${test_dir})
        add_test(
            NAME ${test}
            COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/Interaction_test.py
                -e ${CMAKE_BINARY_DIR}/vp
                -l ${CMAKE_SOURCE_DIR}/configs/fw/${conf_lua}
                -i ${test_dir}
                -- ${extra}
        )
        set_tests_properties(${test} PROPERTIES TIMEOUT 600)
    else()
        message(WARNING "Skipping test ${test}, could not find '${test_dir}'. To enable tests, set QQVP_FIRMWARE_REPO_DIR in environment.")
    endif()
endmacro()

set(SYNC --param platform.ArmQemuInstance.sync_policy=\"multithread-quantum\")
gs_add_test(Makena_fastrpc_calc_test_quantum 8540/SA8540P_conf.lua "makena/default" "${SYNC}")
gs_add_test(LeMans_fastrpc_calc_test_quantum 8775/SA8775P_conf.lua "lemans" "${SYNC}")
gs_add_test(Makena_fastrpc_calc_test 8540/SA8540P_conf.lua "makena/default" "")
gs_add_test(LeMans_fastrpc_calc_test 8775/SA8775P_conf.lua "lemans" "")

foreach(flavor IN ITEMS opengl red_hat)
    if(EXISTS ${QQVP_FIRMWARE_REPO_DIR}/${flavor}_linux)
        if("${flavor}" STREQUAL "red_hat")
            if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
                # NEEDSWORK: the red-hat boot test currently times out in Debug builds.
                continue()
            endif()
            set(EXTRA_OPTS -i "${CMAKE_SOURCE_DIR}/configs/fw/8775/bsp/lnx/lrh"
                           -l "${CMAKE_SOURCE_DIR}/configs/fw/8775/SA8775P_conf.lua"
                           -v "LRH_IMAGES_DIR=${QQVP_FIRMWARE_REPO_DIR}/red_hat_linux/")
        else()
            set(EXTRA_OPTS -i "${QQVP_FIRMWARE_REPO_DIR}/${flavor}_linux/")
        endif()
        add_test(
            NAME Linux_${flavor}_boot_test
            COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/Linux_test.py -e ${CMAKE_BINARY_DIR}/vp ${EXTRA_OPTS}
            WORKING_DIRECTORY "${QQVP_FIRMWARE_REPO_DIR}/${flavor}_linux/"
        )
    else()
        message(WARNING "Skipping Linux_${flavor}_boot_test, could not find '${QQVP_FIRMWARE_REPO_DIR}/${flavor}_linux'. To enable tests, set QQVP_FIRMWARE_REPO_DIR in environment.")
    endif()
endforeach()

