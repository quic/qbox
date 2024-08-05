gs_create_dymod(vhost_user_vsock_pci)

target_include_directories(
    vhost_user_vsock_pci PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/qemu-components/pci/qemu_gpex/include>
  )
