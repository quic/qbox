set(LIBQEMU_TARGETS aarch64)
set(EXECUTABLE_PATH "${CMAKE_BINARY_DIR}")

message(STATUS "EXECUTABLE_PATH: ${EXECUTABLE_PATH}")

add_definitions(-DEXECUTABLE_PATH="${EXECUTABLE_PATH}")

add_executable(cortex-m55-vp
    src/main.cc
)

add_executable(remote_cpu
    src/remote_cpu.cc
)

target_link_libraries(cortex-m55-vp
    ${TARGET_LIBS}
)

target_link_libraries(remote_cpu
    ${TARGET_LIBS}
)

target_include_directories(
    cortex-m55-vp PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/systemc-components>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/qemu-components>
  )

  target_include_directories(
    remote_cpu PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/systemc-components>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/qemu-components>
  )

gs_enable_testing()