gs_create_dymod(display)

target_include_directories(
    display PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/qemu-components/pci>
  )
