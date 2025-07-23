qbox_add_cpu_test(aarch64-simple-write-test 100 simple-write-test.cc)
qbox_add_cpu_test(aarch64-dmi-test 100 dmi-test.cc)
qbox_add_cpu_test(aarch64-dmi-test-concurrent-inval 100 dmi-test-concurrent-inval.cc)
# Build assembly firmware for DMI reset test
find_program(LLVM_MC llvm-mc)
find_program(LLD ld.lld)
find_program(LLVM_OBJCOPY llvm-objcopy)

# "and APPLE" workaround: QQVPQSP-714
if(LLVM_MC AND LLD AND LLVM_OBJCOPY AND APPLE)
    # Assemble the ARM64 assembly file
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/dmi-test-reset.o
        COMMAND ${LLVM_MC} -arch=aarch64 -filetype=obj
                ${CMAKE_CURRENT_SOURCE_DIR}/dmi-test-reset.S
                -o ${CMAKE_CURRENT_BINARY_DIR}/dmi-test-reset.o
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/dmi-test-reset.S
        COMMENT "Assembling dmi-test-reset.S"
    )

    # Link the object file
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/dmi-test-reset.elf
        COMMAND ${LLD} -T ${CMAKE_CURRENT_SOURCE_DIR}/dmi-test-reset.ld
                ${CMAKE_CURRENT_BINARY_DIR}/dmi-test-reset.o
                -o ${CMAKE_CURRENT_BINARY_DIR}/dmi-test-reset.elf
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/dmi-test-reset.o
                ${CMAKE_CURRENT_SOURCE_DIR}/dmi-test-reset.ld
        COMMENT "Linking dmi-test-reset.elf"
    )

    # Create binary file
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/dmi-test-reset.bin
        COMMAND ${LLVM_OBJCOPY} -O binary
                ${CMAKE_CURRENT_BINARY_DIR}/dmi-test-reset.elf
                ${CMAKE_CURRENT_BINARY_DIR}/dmi-test-reset.bin
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/dmi-test-reset.elf
        COMMENT "Creating dmi-test-reset.bin"
    )

    # Create a custom target for the firmware
    add_custom_target(dmi-test-reset-firmware
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/dmi-test-reset.bin)

    qbox_add_cpu_test(aarch64-dmi-test-reset 100 dmi-test-reset.cc)
    add_dependencies(aarch64-dmi-test-reset dmi-test-reset-firmware)
    target_compile_definitions(aarch64-dmi-test-reset PRIVATE
        FIRMWARE_BIN_PATH="${CMAKE_CURRENT_BINARY_DIR}/dmi-test-reset.bin")
else()
    message(WARNING "LLVM tools not found - skipping aarch64-dmi-test-reset")
endif()
qbox_add_cpu_test(aarch64-ld-st-excl-fail-test 100 ld-st-excl-fail.cc)
qbox_add_cpu_test(aarch64-write_read 100 write_read.cc)
qbox_add_cpu_test(aarch64-dmi-test-async-inval 500 dmi-test-async-inval.cc)
