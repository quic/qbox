gs_create_dymod(virtio_gpu_pci)

target_include_directories(
    virtio_gpu_pci PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/qemu-components/pci>
  )
