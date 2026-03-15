# Build assembly firmware for RISC-V 64-bit vector test
find_program(LLVM_MC llvm-mc)
find_program(LLD ld.lld)
find_program(LLVM_OBJCOPY llvm-objcopy)

if(LLVM_MC AND LLD AND LLVM_OBJCOPY)
    # Assemble the RISC-V 64-bit vector test assembly file
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/riscv64-vector-test.o
        COMMAND ${LLVM_MC} -arch=riscv64 -mattr=+v -filetype=obj
                ${CMAKE_CURRENT_SOURCE_DIR}/riscv64-vector-test.S
                -o ${CMAKE_CURRENT_BINARY_DIR}/riscv64-vector-test.o
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/riscv64-vector-test.S
        COMMENT "Assembling riscv64-vector-test.S"
    )

    # Link the object file
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/riscv64-vector-test.elf
        COMMAND ${LLD} -T ${CMAKE_CURRENT_SOURCE_DIR}/riscv64-vector-test.ld
                ${CMAKE_CURRENT_BINARY_DIR}/riscv64-vector-test.o
                -o ${CMAKE_CURRENT_BINARY_DIR}/riscv64-vector-test.elf
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/riscv64-vector-test.o
                ${CMAKE_CURRENT_SOURCE_DIR}/riscv64-vector-test.ld
        COMMENT "Linking riscv64-vector-test.elf"
    )

    # Create binary file
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/riscv64-vector-test.bin
        COMMAND ${LLVM_OBJCOPY} -O binary
                ${CMAKE_CURRENT_BINARY_DIR}/riscv64-vector-test.elf
                ${CMAKE_CURRENT_BINARY_DIR}/riscv64-vector-test.bin
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/riscv64-vector-test.elf
        COMMENT "Creating riscv64-vector-test.bin"
    )

    # Create a custom target for the firmware
    add_custom_target(riscv64-vector-test-firmware
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/riscv64-vector-test.bin)

    # Simple single-CPU RISC-V vector test
    add_executable(riscv64-vector-test riscv64-vector-test.cc)
    add_dependencies(riscv64-vector-test riscv64-vector-test-firmware)
    target_compile_definitions(riscv64-vector-test PRIVATE
        FIRMWARE_BIN_PATH="${CMAKE_CURRENT_BINARY_DIR}/riscv64-vector-test.bin")
    target_include_directories(riscv64-vector-test PRIVATE
        ${PROJECT_SOURCE_DIR}/tests/libqbox/include
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/qemu-components/cpu_riscv/cpu_riscv64/include>
        ${keystone_SOURCE_DIR}/include)
    target_link_libraries(riscv64-vector-test PRIVATE
        cpu_riscv64 router reset_gpio gs_memory keystone ${TARGET_LIBS})

    # Add single test configuration
    add_test(
        NAME riscv64-vector-test
        COMMAND riscv64-vector-test --param test-bench.inst_a.cpu_1.vector=true
    )
    set_tests_properties(riscv64-vector-test PROPERTIES TIMEOUT 30)
else()
    message(WARNING "LLVM tools not found - skipping riscv64-vector-test")
endif()
