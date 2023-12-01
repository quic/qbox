gs_create_dymod(rtl8139_pci)

target_include_directories(
    rtl8139_pci PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/qemu-components/pci/qemu_gpex/include>
  )
