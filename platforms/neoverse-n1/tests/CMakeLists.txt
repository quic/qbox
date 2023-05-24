
set(QBOX_FIRMWARE_REPO_DIR "$ENV{QBOX_FIRMWARE_REPO_DIR}" CACHE STRING "")

macro(gs_add_test test conf_lua image_dir)
    set(test_dir "${QBOX_FIRMWARE_REPO_DIR}/${image_dir}")
    if(EXISTS ${test_dir})
        add_test(
            NAME ${test}
            COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/neoverse_n1.py
                -e ${CMAKE_BINARY_DIR}/platforms/neoverse-n1/neoverse-n1-vp
                -l ${CMAKE_SOURCE_DIR}/platforms/neoverse-n1/${conf_lua}
                -i ${test_dir}
        )
        set_tests_properties(${test} PROPERTIES TIMEOUT 30)
    else()
        message(WARNING "Skipping test ${test}, could not find '${test_dir}'. To enable tests, set QBOX_FIRMWARE_REPO_DIR in environment.")
    endif()
endmacro()

gs_add_test(neoverse_n1 conf.lua "neoverse-n1")