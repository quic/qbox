gs_create_dymod(qemu_cpu_hexagon)

target_include_directories(
    qemu_cpu_hexagon PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/qemu-components/hexagon_globalreg/include>
)
