gs_create_dymod(gs_gpex)

target_include_directories(
    gs_gpex PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/qemu-components/common/include>
)