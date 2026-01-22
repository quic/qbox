gs_create_dymod(vnc)

target_include_directories(
    vnc PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/qemu-components/pci>
  )
