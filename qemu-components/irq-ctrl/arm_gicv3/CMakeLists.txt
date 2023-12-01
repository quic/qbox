gs_create_dymod(arm_gicv3)

target_include_directories(
    arm_gicv3 PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/qemu-components/global_peripheral_initiator/include>
  )
