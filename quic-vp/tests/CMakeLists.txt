
set(QQVP_FIRMWARE_REPO_DIR "$ENV{QQVP_FIRMWARE_REPO_DIR}" CACHE STRING "")

macro(gs_add_test test conf_lua image_dir)
    set(test_dir "${QQVP_FIRMWARE_REPO_DIR}/${image_dir}")
    if(EXISTS ${test_dir})
        add_test(
            NAME ${test}
            COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/Interaction_test.py -e ${CMAKE_BINARY_DIR}/vp -l ${CMAKE_SOURCE_DIR}/../configs/${conf_lua} -i ${test_dir}
        )
        set_tests_properties(${test} PROPERTIES TIMEOUT 600)
    else()
        message(WARNING "Skipping test ${test}, could not find '${test_dir}'. To enable tests, set QQVP_FIRMWARE_REPO_DIR in environment.")
    endif()
endmacro()

gs_add_test(Makena_fastrpc_calc_test SA8540P/SA8540P_conf.lua "makena/default")
gs_add_test(LeMans_fastrpc_calc_test SA8775P/SA8775P_conf.lua "lemans")

if(EXISTS ${QQVP_FIRMWARE_REPO_DIR}/opengl_linux)
    add_test(
        NAME Linux_boot_test
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/Linux_test.py -e ${CMAKE_BINARY_DIR}/vp -l "${QQVP_FIRMWARE_REPO_DIR}/opengl_linux/conf.lua" -i "${QQVP_FIRMWARE_REPO_DIR}/opengl_linux/"
        WORKING_DIRECTORY "${QQVP_FIRMWARE_REPO_DIR}/opengl_linux/"
    )
else()
    message(WARNING "Skipping Linux_boot_test, could not find '${QQVP_FIRMWARE_REPO_DIR}/opengl_linux'. To enable tests, set QQVP_FIRMWARE_REPO_DIR in environment.")
endif()
