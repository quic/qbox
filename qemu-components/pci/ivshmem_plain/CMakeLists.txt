gs_create_dymod(ivshmem_plain)

target_include_directories(
    ivshmem_plain PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/qemu-components/pci/qemu_gpex/include>
  )
