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

target_include_directories(
    cortex-m55-vp PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/systemc-components>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/qemu-components>
  )

  target_include_directories(
    remote_cpu PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/systemc-components>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/qemu-components>
  )

set(BUILD_DIRECTORY "${CMAKE_BINARY_DIR}")
configure_file(${PROJECT_SOURCE_DIR}/platforms/cortex-m55-remote/conf.lua.in ${PROJECT_SOURCE_DIR}/platforms/cortex-m55-remote/conf.lua)

gs_enable_testing()