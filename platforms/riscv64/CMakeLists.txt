set(LIBQEMU_TARGETS riscv64)

add_executable(riscv64-base-vp
    src/main.cc
)

if(APPLE)
    target_link_options(riscv64-base-vp PRIVATE "LINKER:-dead_strip_dylibs")
else()
    target_link_options(riscv64-base-vp PRIVATE "LINKER:-as-needed")
endif()

target_link_libraries(riscv64-base-vp
    ${TARGET_LIBS}
)
