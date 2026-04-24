gs_create_dymod(hexagon_globalreg)

target_include_directories(
    hexagon_globalreg PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/qemu-components/timer/qemu_hexagon_qtimer/include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/qemu-components/irq-ctrl/hexagon_l2vic/include>
)
