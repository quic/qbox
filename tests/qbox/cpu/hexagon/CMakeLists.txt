qbox_add_cpu_test(hexagon-ld-st-mmio-test 100 ld-st-mmio.cc)
qbox_add_cpu_test(hexagon-reset-test 100 hex-reset.cc)

set(HEXAGON_STANDALONE_TEST_NAME "load-store-test")

add_executable(
    ${HEXAGON_STANDALONE_TEST_NAME}
    ${HEXAGON_STANDALONE_TEST_NAME}.cc
)

target_include_directories(
    ${HEXAGON_STANDALONE_TEST_NAME}
    PRIVATE
    ${keystone_SOURCE_DIR}/include
)

target_link_libraries(
    ${HEXAGON_STANDALONE_TEST_NAME}
    PRIVATE
    ${TARGET_LIBS}
    keystone
    gs_memory
    router
    qemu_cpu_hexagon
    hexagon_globalreg
)

add_test(NAME hexagon-${HEXAGON_STANDALONE_TEST_NAME} COMMAND ${HEXAGON_STANDALONE_TEST_NAME})

