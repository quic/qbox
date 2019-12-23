cmake_minimum_required (VERSION 3.1)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

project(libqemu-cxx)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(libqemu REQUIRED)

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

add_library(qemu-cxx SHARED ${srcs})

target_include_directories(qemu-cxx
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
        $<INSTALL_INTERFACE:include/libqemu-cxx>
)

target_link_libraries(qemu-cxx libqemu)

install(FILES
	"${CMAKE_CURRENT_SOURCE_DIR}/libqemu-cxx.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/exceptions.h"
	DESTINATION include/libqemu-cxx)
install(FILES
	"${CMAKE_CURRENT_SOURCE_DIR}/target/arm.h"
	DESTINATION include/libqemu-cxx/target)

install(TARGETS qemu-cxx DESTINATION lib)
