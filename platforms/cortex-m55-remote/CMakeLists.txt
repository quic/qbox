cmake_minimum_required(VERSION 3.14)
project(mini-vp)

set(LIBQEMU_TARGETS aarch64)

add_subdirectory(../../ ${CMAKE_BINARY_DIR}/_deps/qbox-build)

add_executable(vp
    src/main.cc
)

add_executable(remote_cpu
    src/remote_cpu.cc
)

target_link_libraries(vp
    ${TARGET_LIBS}
    GreenSocs::qbox
)

target_link_libraries(remote_cpu
    ${TARGET_LIBS}
    GreenSocs::qbox
)

gs_enable_testing()