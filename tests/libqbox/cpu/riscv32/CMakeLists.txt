# Build assembly firmware for RISC-V reset test
find_program(LLVM_MC llvm-mc)
find_program(LLD ld.lld)
find_program(LLVM_OBJCOPY llvm-objcopy)

# "and APPLE" workaround: QQVPQSP-714
if(LLVM_MC AND LLD AND LLVM_OBJCOPY AND APPLE)
    # Assemble the RISC-V 32-bit assembly file
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/riscv32-reset.o
        COMMAND ${LLVM_MC} -arch=riscv32 -filetype=obj
                ${CMAKE_CURRENT_SOURCE_DIR}/riscv32-reset.S
                -o ${CMAKE_CURRENT_BINARY_DIR}/riscv32-reset.o
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/riscv32-reset.S
        COMMENT "Assembling riscv32-reset.S"
    )

    # Link the object file
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/riscv32-reset.elf
        COMMAND ${LLD} -T ${CMAKE_CURRENT_SOURCE_DIR}/riscv32-reset.ld
                ${CMAKE_CURRENT_BINARY_DIR}/riscv32-reset.o
                -o ${CMAKE_CURRENT_BINARY_DIR}/riscv32-reset.elf
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/riscv32-reset.o
                ${CMAKE_CURRENT_SOURCE_DIR}/riscv32-reset.ld
        COMMENT "Linking riscv32-reset.elf"
    )

    # Create binary file
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/riscv32-reset.bin
        COMMAND ${LLVM_OBJCOPY} -O binary
                ${CMAKE_CURRENT_BINARY_DIR}/riscv32-reset.elf
                ${CMAKE_CURRENT_BINARY_DIR}/riscv32-reset.bin
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/riscv32-reset.elf
        COMMENT "Creating riscv32-reset.bin"
    )

    # Create a custom target for the firmware
    add_custom_target(riscv32-reset-firmware
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/riscv32-reset.bin)

    qbox_add_cpu_test(riscv32-reset-test 100 riscv32-reset.cc)
    add_dependencies(riscv32-reset-test riscv32-reset-firmware)
    target_compile_definitions(riscv32-reset-test PRIVATE
        FIRMWARE_BIN_PATH="${CMAKE_CURRENT_BINARY_DIR}/riscv32-reset.bin")
else()
    message(WARNING "LLVM tools not found - skipping riscv32-reset-test")
endif()
