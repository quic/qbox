gs_create_dymod(virtio_net_pci)

target_include_directories(
    virtio_net_pci PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/qemu-components/pci/qemu_gpex/include>
  )
