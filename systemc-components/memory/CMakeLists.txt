gs_create_dymod(memory)

target_link_libraries(memory PRIVATE
    ${LIBELF_LIBRARIES}
)

if(UNIX AND NOT APPLE)
    target_link_libraries(memory PRIVATE rt)
endif()
