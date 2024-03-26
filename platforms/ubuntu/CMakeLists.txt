set(LIBQEMU_TARGETS aarch64)

add_executable(ubuntu-vp
    src/main.cc
)
if(APPLE)
    target_link_options(ubuntu-vp PRIVATE "LINKER:-dead_strip_dylibs")
else()
    target_link_options(ubuntu-vp PRIVATE "LINKER:-as-needed")
endif()

target_link_libraries(ubuntu-vp
    ${TARGET_LIBS}
    GreenSocs::qbox
)