set(LIBQEMU_TARGETS aarch64)

add_executable(cortex-m55-vp
    src/main.cc
)

add_executable(remote_cpu
    src/remote_cpu.cc
)

target_link_libraries(cortex-m55-vp
    ${TARGET_LIBS}
    GreenSocs::qbox
)

target_link_libraries(remote_cpu
    ${TARGET_LIBS}
    GreenSocs::qbox
)

gs_enable_testing()