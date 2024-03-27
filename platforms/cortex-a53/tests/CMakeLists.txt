
set(QBOX_FIRMWARE_REPO_DIR "$ENV{QBOX_FIRMWARE_REPO_DIR}" CACHE STRING "")

macro(gs_add_test test conf_lua image_dir)
    set(test_dir "${QBOX_FIRMWARE_REPO_DIR}/${image_dir}")
    if(EXISTS ${test_dir})
        add_test(
            NAME ${test}
            COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/cortex_a53.py
                -e ${CMAKE_BINARY_DIR}/platforms/platforms-vp
                -l ${CMAKE_SOURCE_DIR}/platforms/cortex-a53/${conf_lua}
                -i ${test_dir}
        )
        set_tests_properties(${test} PROPERTIES TIMEOUT 60)
    else()
        message(WARNING "Skipping test ${test}, could not find '${test_dir}'. To enable tests, set QBOX_FIRMWARE_REPO_DIR in environment.")
    endif()
endmacro()

gs_add_test(cortex_a53 conf.lua "cortex-a53")