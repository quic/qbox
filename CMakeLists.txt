cmake_minimum_required (VERSION 3.1)

project(libqemu-cxx)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(libqemu REQUIRED)

if (NOT DEFINED LIBQEMU_TARGETS)
    set(LIBQEMU_TARGETS arm)
endif()

set(srcs
    cpu.cc
    device.cc
    gpio.cc
    libqemu-cxx.cc
    memory.cc
    sysbus.cc
    timer.cc
    target/aarch64.cc
    target/riscv32.cc
    target/riscv64.cc
    target/microblaze.cc
    ${CMAKE_CURRENT_BINARY_DIR}/target_info.cc
)

foreach(target ${LIBQEMU_TARGETS})
    # This variable is used in target_info.cc.in
    get_property(LIBQEMU_TARGET_${target}_LIBRARY
        TARGET libqemu-${target}
        PROPERTY INTERFACE_LINK_LIBRARIES)
endforeach()

configure_file(target_info.cc.in target_info.cc)

add_library(qemu-cxx SHARED ${srcs})

target_compile_options(qemu-cxx PRIVATE
    -Wall
    "$<$<CONFIG:Debug>:-O0;-g3;-ggdb>"
)

target_include_directories(qemu-cxx
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
        $<INSTALL_INTERFACE:include/libqemu-cxx>
)

target_link_libraries(qemu-cxx libqemu)

install(FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/exceptions.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/libqemu-cxx.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/target_info.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/target/aarch64.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/target/riscv32.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/target/riscv64.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/target/microblaze.h"
    DESTINATION ${LIBQEMU_CXX_INCLUDE_DIR})

if (WIN32)
    install(TARGETS qemu-cxx DESTINATION lib/$<IF:$<CONFIG:Debug>,debug,release>)
else ()
    install(TARGETS qemu-cxx DESTINATION lib)
endif ()
