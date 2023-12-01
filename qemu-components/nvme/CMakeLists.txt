gs_create_dymod(nvme)

target_include_directories(
    nvme PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/qemu-components/pci/qemu_gpex/include>
  )