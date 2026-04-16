gs_create_dymod(qemu_cpu_hexagon)

target_include_directories(
    qemu_cpu_hexagon PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/qemu-components/hexagon_globalreg/include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/qemu-components/irq-ctrl/hexagon_l2vic/include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/qemu-components/hexagon_tlb/include>
)

target_link_libraries(qemu_cpu_hexagon PRIVATE hexagon_globalreg hexagon_tlb)
