add_executable(vp-hex-remote 
    ${CMAKE_SOURCE_DIR}/rplugins/hexagon/src/remote_hex.cc
)
target_link_libraries(vp-hex-remote 
    ${TARGET_LIBS}
)
set(QQVP_FIRMWARE_REPO_DIR "$ENV{QQVP_FIRMWARE_REPO_DIR}" CACHE STRING "")

macro(gs_add_test test conf_lua image_dir plugin_dir)
    set(test_dir "${QQVP_FIRMWARE_REPO_DIR}/${image_dir}")
    if(EXISTS ${test_dir})
        add_test(
            NAME ${test}
            COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/Interaction_test.py -e ${CMAKE_BINARY_DIR}/vp -l ${CMAKE_SOURCE_DIR}/configs/${conf_lua} -i ${test_dir} -p ${CMAKE_SOURCE_DIR}/${plugin_dir} 
        )
        set_tests_properties(${test} PROPERTIES TIMEOUT 600)
    else()
        message(WARNING "Skipping test ${test}, could not find '${test_dir}'. To enable tests, set QQVP_FIRMWARE_REPO_DIR in environment.")
    endif()
endmacro()

gs_add_test(Remote-hex-LeMans_fastrpc_calc_test SA8775P/SA8775P_conf.lua "lemans" rplugins/hexagon/config)