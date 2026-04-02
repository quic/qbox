if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    # -----------------------------------------------------------
    # Standalone firmware build (cross-compilation only).
    # Run from this directory:
    #   cmake -B build .
    #   cmake --build build
    # Produces hello.elf next to platform.lua.
    # -----------------------------------------------------------
    cmake_minimum_required(VERSION 3.14)

    set(CMAKE_SYSTEM_NAME Generic)
    set(CMAKE_SYSTEM_PROCESSOR aarch64)
    set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
    # Compiler verification builds a static library instead of
    # an executable, avoiding the need to run the cross-compiled
    # output.
    set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

    project(hello-qbox-firmware C)

    add_executable(hello hello.c)
    set_target_properties(hello PROPERTIES SUFFIX ".elf")
    target_link_options(hello PRIVATE
        -nostdlib
        -static
        -Ttext=0x80000000
        -e _start
        -Wl,--build-id=none
        -Wl,-n
    )

else()
    # -----------------------------------------------------------
    # Part of the qbox build.
    # Produces the hello-qbox-vp binary and, if the AArch64
    # cross-compiler is available, also cross-compiles hello.elf.
    # -----------------------------------------------------------

    # --- VP binary -------------------------------------------
    add_executable(hello-qbox-vp
        ${CMAKE_CURRENT_SOURCE_DIR}/main.cc
    )

    if(APPLE)
        target_link_options(hello-qbox-vp PRIVATE
            "LINKER:-dead_strip_dylibs")
    else()
        target_link_options(hello-qbox-vp PRIVATE
            "LINKER:-as-needed")
    endif()

    target_link_libraries(hello-qbox-vp PRIVATE
        ${TARGET_LIBS}
    )

    set_target_properties(hello-qbox-vp PROPERTIES
        INSTALL_RPATH
        "$ORIGIN/../../../lib;$ORIGIN/../../../lib/libqemu"
    )


    # --- Firmware (AArch64 cross-compilation) ----------------
    find_program(AARCH64_GCC aarch64-linux-gnu-gcc)

    if(AARCH64_GCC)
        set(HELLO_ELF
            ${CMAKE_CURRENT_SOURCE_DIR}/build/hello.elf)

        add_custom_command(
            OUTPUT ${HELLO_ELF}
            COMMAND ${CMAKE_COMMAND} -E make_directory
                ${CMAKE_CURRENT_SOURCE_DIR}/build
            COMMAND ${AARCH64_GCC}
                -nostdlib -static
                -Ttext=0x80000000 -e _start
                -Wl,--build-id=none
                -Wl,-n
                -o ${HELLO_ELF}
                ${CMAKE_CURRENT_SOURCE_DIR}/hello.c
            DEPENDS
                ${CMAKE_CURRENT_SOURCE_DIR}/hello.c
            COMMENT "Cross-compiling hello.elf for AArch64"
        )

        add_custom_target(hello-qbox-firmware ALL
            DEPENDS ${HELLO_ELF}
        )
    else()
        message(WARNING
            "aarch64-linux-gnu-gcc not found; "
            "hello.elf must be built manually. "
            "See examples/hello-qbox/README.md.")
    endif()

endif()
