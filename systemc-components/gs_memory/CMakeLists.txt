gs_create_dymod(gs_memory)

target_link_libraries(gs_memory PRIVATE
    ${LIBELF_LIBRARIES}
)

if(UNIX AND NOT APPLE)
    target_link_libraries(gs_memory PRIVATE rt)
endif()
