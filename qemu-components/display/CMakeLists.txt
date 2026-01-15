gs_create_dymod(display)

target_include_directories(
    display PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/qemu-components/pci>
  )
  target_include_directories(
    display PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/qemu-components/ramfb/include>
  )
  target_include_directories(
    display PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/qemu-components/fw_cfg/include>
  )
