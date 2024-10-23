gs_create_dymod(monitor)

add_custom_command(TARGET monitor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E create_symlink
    ${CMAKE_CURRENT_SOURCE_DIR}/static/monitor.html $<TARGET_FILE_DIR:monitor>/monitor.html)

install (FILES ${CMAKE_CURRENT_SOURCE_DIR}/static/monitor.html DESTINATION bin)