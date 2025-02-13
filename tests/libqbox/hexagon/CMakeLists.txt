# Copyright (c) 2025 Qualcomm Innovation Center, Inc. All Rights Reserved.
# SPDX-License-Identifier: BSD-3-Clause

set(TEST_NAME "load-store-test")

add_executable(
    ${TEST_NAME}
    ${TEST_NAME}.cc
)

target_include_directories(
    ${TEST_NAME}
    PRIVATE
    ${keystone_SOURCE_DIR}/include
)

target_link_libraries(
    ${TEST_NAME}
    PRIVATE
    ${TARGET_LIBS}
    keystone
    gs_memory
    router
    qemu_cpu_hexagon
)

set(CTEST_PREFIX "hexagon")

add_test(${CTEST_PREFIX}-${TEST_NAME} ${TEST_NAME})
