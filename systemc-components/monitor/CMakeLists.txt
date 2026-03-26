gs_create_dymod(monitor)

add_custom_command(TARGET monitor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/static $<TARGET_FILE_DIR:monitor>/static)

install (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/static DESTINATION ${CMAKE_BINARY_DIR})
