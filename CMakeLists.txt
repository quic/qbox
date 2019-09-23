cmake_minimum_required (VERSION 3.1)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

project(libqemu-cxx)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(libqemu REQUIRED)

include_directories(${LIBQEMU_INCLUDE_DIR})

set(srcs
    cpu.cc
    device.cc
    gpio.cc
    libqemu-cxx.cc
    memory.cc
    sysbus.cc
    timer.cc
    target/arm.cc
)

add_library(qemu-cxx ${srcs})
