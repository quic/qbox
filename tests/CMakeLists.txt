macro(gs_add_test test conf_lua image_dir)
    if(DEFINED ENV{QQVP_FIRMWARE_REPO_DIR})
        set(IMAGE_DIR_BASE "$ENV{QQVP_FIRMWARE_REPO_DIR}")
        add_test(
            NAME ${test}
            COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/Interaction_test.py -e ${CMAKE_SOURCE_DIR}/build/vp -l ${CMAKE_SOURCE_DIR}/${conf_lua} -i ${IMAGE_DIR_BASE}/${image_dir}
        )
        set_tests_properties(${test} PROPERTIES TIMEOUT 120)
    else()
        message(WARNING "Skipping test ${test}. To enable tests, set QQVP_FIRMWARE_REPO_DIR in environment.")
    endif()
endmacro()

gs_add_test(Makena_fastrpc_calc_test SA8540P_conf.lua "makena/default")
#gs_add_test(LeMans_fastrpc_calc_test SA8775P_conf.lua "lemans")
