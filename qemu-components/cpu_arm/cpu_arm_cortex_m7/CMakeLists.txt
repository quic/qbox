gs_create_dymod(cpu_arm_cortexM7)

target_include_directories(
    cpu_arm_cortexM7 PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/qemu-components/irq-ctrl/armv7m_nvic/include>
  )
