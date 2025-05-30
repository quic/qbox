set(LIBQEMU_TARGETS aarch64)

add_executable(cortex-m55-vp
    src/main.cc
)

add_executable(remote_cpu
    src/remote_cpu.cc
)

target_link_libraries(cortex-m55-vp
    ${TARGET_LIBS}
)

target_link_libraries(remote_cpu
    ${TARGET_LIBS}
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

set_target_properties(cortex-m55-vp PROPERTIES
  INSTALL_RPATH "$ORIGIN/../lib;$ORIGIN/../lib/libqemu;$ORIGIN/../../../lib"
)

set_target_properties(remote_cpu PROPERTIES
  INSTALL_RPATH "$ORIGIN/../lib;$ORIGIN/../lib/libqemu;$ORIGIN/../../../lib"
)

install(TARGETS cortex-m55-vp RUNTIME DESTINATION bin/platforms/cortex-m55-remote)
install(TARGETS remote_cpu RUNTIME DESTINATION bin/platforms/cortex-m55-remote)

gs_enable_testing()