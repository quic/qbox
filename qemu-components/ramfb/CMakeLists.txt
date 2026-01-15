gs_create_dymod(ramfb)
target_include_directories(
    ramfb PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/qemu-components/fw_cfg/include>
  )
