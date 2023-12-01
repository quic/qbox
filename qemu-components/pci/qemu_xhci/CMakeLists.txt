gs_create_dymod(qemu_xhci)

target_include_directories(
    qemu_xhci PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/qemu-components/pci/qemu_gpex/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}>
  )
