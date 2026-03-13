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

# Build assembly firmware for Hexagon SMMU stress test
find_program(LLVM_MC llvm-mc)
find_program(LLD ld.lld)
find_program(LLVM_OBJCOPY llvm-objcopy)

# Check if LLVM tools are available for building Hexagon firmware
if(LLVM_MC AND LLD AND LLVM_OBJCOPY AND APPLE)
    # Assemble the Hexagon assembly file
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/hexagon_smmu_firmware.o
        COMMAND ${LLVM_MC} -arch=hexagon -filetype=obj
                ${CMAKE_CURRENT_SOURCE_DIR}/hexagon_smmu_firmware.s
                -o ${CMAKE_CURRENT_BINARY_DIR}/hexagon_smmu_firmware.o
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/hexagon_smmu_firmware.s
        COMMENT "Assembling hexagon_smmu_firmware.s"
    )

    # Link the object file (with optional symbol overrides)
    # Note: You can override constants by adding --defsym options like:
    # --defsym TESTER_ADDR=0x50000 --defsym PAGE_SIZE=0x2000
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/hexagon_smmu_firmware.elf
        COMMAND ${LLD} -T ${CMAKE_CURRENT_SOURCE_DIR}/hexagon_smmu_firmware.ld
                ${CMAKE_CURRENT_BINARY_DIR}/hexagon_smmu_firmware.o
                -o ${CMAKE_CURRENT_BINARY_DIR}/hexagon_smmu_firmware.elf
                --defsym TESTER_ADDR=0x40000
                --defsym REG_DEBUG=0x28
                --defsym PAGE_SIZE=0x1000
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/hexagon_smmu_firmware.o
                ${CMAKE_CURRENT_SOURCE_DIR}/hexagon_smmu_firmware.ld
        COMMENT "Linking hexagon_smmu_firmware.elf (demonstrating --defsym capability)"
    )

    # Create binary file
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/hexagon_smmu_firmware.bin
        COMMAND ${LLVM_OBJCOPY} -O binary
                ${CMAKE_CURRENT_BINARY_DIR}/hexagon_smmu_firmware.elf
                ${CMAKE_CURRENT_BINARY_DIR}/hexagon_smmu_firmware.bin
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/hexagon_smmu_firmware.elf
        COMMENT "Creating hexagon_smmu_firmware.bin"
    )

    # Create a custom target for the firmware
    add_custom_target(hexagon-smmu-firmware
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/hexagon_smmu_firmware.bin)

    qbox_add_cpu_test(hexagon-smmu-stress-test-v2 100 hexagon_smmu_stress_test_v2.cc)
    add_dependencies(hexagon-smmu-stress-test-v2 hexagon-smmu-firmware)
    target_compile_definitions(hexagon-smmu-stress-test-v2 PRIVATE
        FIRMWARE_BIN_PATH="${CMAKE_CURRENT_BINARY_DIR}/hexagon_smmu_firmware.bin")
else()
    message(WARNING "LLVM tools not found - skipping hexagon-smmu-stress-test-v2")
endif()
