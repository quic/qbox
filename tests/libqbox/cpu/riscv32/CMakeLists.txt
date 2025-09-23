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

    # Build assembly firmware for RISC-V DMI test
    # Assemble the RISC-V 32-bit DMI test assembly file
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/riscv32-dmi-test.o
        COMMAND ${LLVM_MC} -arch=riscv32 -filetype=obj
                ${CMAKE_CURRENT_SOURCE_DIR}/riscv32-dmi-test.S
                -o ${CMAKE_CURRENT_BINARY_DIR}/riscv32-dmi-test.o
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/riscv32-dmi-test.S
        COMMENT "Assembling riscv32-dmi-test.S"
    )

    # Link the DMI test object file
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/riscv32-dmi-test.elf
        COMMAND ${LLD} -T ${CMAKE_CURRENT_SOURCE_DIR}/riscv32-dmi-test.ld
                ${CMAKE_CURRENT_BINARY_DIR}/riscv32-dmi-test.o
                -o ${CMAKE_CURRENT_BINARY_DIR}/riscv32-dmi-test.elf
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/riscv32-dmi-test.o
                ${CMAKE_CURRENT_SOURCE_DIR}/riscv32-dmi-test.ld
        COMMENT "Linking riscv32-dmi-test.elf"
    )

    # Create DMI test binary file
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/riscv32-dmi-test.bin
        COMMAND ${LLVM_OBJCOPY} -O binary
                ${CMAKE_CURRENT_BINARY_DIR}/riscv32-dmi-test.elf
                ${CMAKE_CURRENT_BINARY_DIR}/riscv32-dmi-test.bin
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/riscv32-dmi-test.elf
        COMMENT "Creating riscv32-dmi-test.bin"
    )

    # Create a custom target for the DMI test firmware
    add_custom_target(riscv32-dmi-test-firmware
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/riscv32-dmi-test.bin)

    qbox_add_cpu_test(riscv32-dmi-test 100 riscv32-dmi-test.cc)
    add_dependencies(riscv32-dmi-test riscv32-dmi-test-firmware)
    target_compile_definitions(riscv32-dmi-test PRIVATE
        FIRMWARE_BIN_PATH="${CMAKE_CURRENT_BINARY_DIR}/riscv32-dmi-test.bin")

    # Build assembly firmware for RISC-V IRQ test
    # Assemble the RISC-V 32-bit IRQ test assembly file
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/riscv32-irq-test.o
        COMMAND ${LLVM_MC} -arch=riscv32 -filetype=obj
                ${CMAKE_CURRENT_SOURCE_DIR}/riscv32-irq-test.S
                -o ${CMAKE_CURRENT_BINARY_DIR}/riscv32-irq-test.o
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/riscv32-irq-test.S
        COMMENT "Assembling riscv32-irq-test.S"
    )

    # Link the IRQ test object file
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/riscv32-irq-test.elf
        COMMAND ${LLD} -T ${CMAKE_CURRENT_SOURCE_DIR}/riscv32-irq-test.ld
                ${CMAKE_CURRENT_BINARY_DIR}/riscv32-irq-test.o
                -o ${CMAKE_CURRENT_BINARY_DIR}/riscv32-irq-test.elf
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/riscv32-irq-test.o
                ${CMAKE_CURRENT_SOURCE_DIR}/riscv32-irq-test.ld
        COMMENT "Linking riscv32-irq-test.elf"
    )

    # Create IRQ test binary file
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/riscv32-irq-test.bin
        COMMAND ${LLVM_OBJCOPY} -O binary
                ${CMAKE_CURRENT_BINARY_DIR}/riscv32-irq-test.elf
                ${CMAKE_CURRENT_BINARY_DIR}/riscv32-irq-test.bin
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/riscv32-irq-test.elf
        COMMENT "Creating riscv32-irq-test.bin"
    )

    # Create a custom target for the IRQ test firmware
    add_custom_target(riscv32-irq-test-firmware
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/riscv32-irq-test.bin)

    qbox_add_cpu_test(riscv32-irq-test 100 riscv32-irq-test.cc)
    add_dependencies(riscv32-irq-test riscv32-irq-test-firmware)
    target_compile_definitions(riscv32-irq-test PRIVATE
        FIRMWARE_BIN_PATH="${CMAKE_CURRENT_BINARY_DIR}/riscv32-irq-test.bin")
    target_include_directories(riscv32-irq-test PRIVATE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/qemu-components/timer/riscv_aclint_mtimer/include>)

    # Build assembly firmware for RISC-V QTimer test
    # Assemble the RISC-V 32-bit QTimer test assembly file
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/riscv32-qtimer-test.o
        COMMAND ${LLVM_MC} -arch=riscv32 -filetype=obj
                ${CMAKE_CURRENT_SOURCE_DIR}/riscv32-qtimer-test.S
                -o ${CMAKE_CURRENT_BINARY_DIR}/riscv32-qtimer-test.o
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/riscv32-qtimer-test.S
        COMMENT "Assembling riscv32-qtimer-test.S"
    )

    # Link the QTimer test object file
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/riscv32-qtimer-test.elf
        COMMAND ${LLD} -T ${CMAKE_CURRENT_SOURCE_DIR}/riscv32-qtimer-test.ld
                ${CMAKE_CURRENT_BINARY_DIR}/riscv32-qtimer-test.o
                -o ${CMAKE_CURRENT_BINARY_DIR}/riscv32-qtimer-test.elf
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/riscv32-qtimer-test.o
                ${CMAKE_CURRENT_SOURCE_DIR}/riscv32-qtimer-test.ld
        COMMENT "Linking riscv32-qtimer-test.elf"
    )

    # Create QTimer test binary file
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/riscv32-qtimer-test.bin
        COMMAND ${LLVM_OBJCOPY} -O binary
                ${CMAKE_CURRENT_BINARY_DIR}/riscv32-qtimer-test.elf
                ${CMAKE_CURRENT_BINARY_DIR}/riscv32-qtimer-test.bin
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/riscv32-qtimer-test.elf
        COMMENT "Creating riscv32-qtimer-test.bin"
    )

    # Create a custom target for the QTimer test firmware
    add_custom_target(riscv32-qtimer-test-firmware
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/riscv32-qtimer-test.bin)

    qbox_add_cpu_test(riscv32-qtimer-test 100 riscv32-qtimer-test.cc)
    add_dependencies(riscv32-qtimer-test riscv32-qtimer-test-firmware)
    target_compile_definitions(riscv32-qtimer-test PRIVATE
        FIRMWARE_BIN_PATH="${CMAKE_CURRENT_BINARY_DIR}/riscv32-qtimer-test.bin")
    target_include_directories(riscv32-qtimer-test PRIVATE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/qemu-components/timer/riscv_aclint_mtimer/include>)
else()
    message(WARNING "LLVM tools not found - skipping riscv32-reset-test, riscv32-dmi-test, riscv32-irq-test, and riscv32-qtimer-test")
endif()
