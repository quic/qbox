gs_create_dymod(virtio_gpu_gl_pci)

target_include_directories(
    virtio_gpu_gl_pci PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/qemu-components/pci>
  )