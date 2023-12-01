gs_create_dymod(qemu_tablet)

target_include_directories(
    qemu_tablet PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/qemu-components/pci/qemu_gpex/include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/qemu-components/pci/qemu_xhci/include>
  )
