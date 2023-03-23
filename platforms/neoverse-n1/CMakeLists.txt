set(LIBQEMU_TARGETS aarch64)

add_executable(neoverse-n1-vp
    src/main.cc
)
if(APPLE)
    target_link_options(neoverse-n1-vp PRIVATE "LINKER:-dead_strip_dylibs")
else()
    target_link_options(neoverse-n1-vp PRIVATE "LINKER:-as-needed")
endif()

target_link_libraries(neoverse-n1-vp
    ${TARGET_LIBS}
    GreenSocs::qbox
)

gs_enable_testing()
