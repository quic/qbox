set(LIBQEMU_TARGETS aarch64)

add_executable(platforms-vp
    ${PROJECT_SOURCE_DIR}/platforms/src/main.cc
)
if(APPLE)
    target_link_options(platforms-vp PRIVATE "LINKER:-dead_strip_dylibs")
else()
    target_link_options(platforms-vp PRIVATE "LINKER:-as-needed")
endif()

target_link_libraries(platforms-vp
    ${TARGET_LIBS}
    GreenSocs::qbox
)

add_subdirectory(cortex-a53)
add_subdirectory(cortex-m55-remote)
add_subdirectory(neoverse-n1)