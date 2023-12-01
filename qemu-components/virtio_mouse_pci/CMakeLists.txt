gs_create_dymod(virtio_mouse_pci)

target_include_directories(
    virtio_mouse_pci PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/qemu-components/pci/qemu_gpex/include>
  )
