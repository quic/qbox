cmake_minimum_required (VERSION 3.1)

project(libqemu-cxx)

include(GNUInstallDirs)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(libqemu CONFIG REQUIRED)

set(LIBQEMU_CXX_SRC_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include/libqemu-cxx)
set(LIBQEMU_CXX_INCLUDE_DIR ${CMAKE_INSTALL_INCLUDEDIR}/libqemu-cxx)
set(LIBQEMU_CXX_LIB_DIR ${CMAKE_INSTALL_LIBDIR}/libqemu-cxx)

set(srcs
    src/cpu.cc
    src/device.cc
    src/gpio.cc
    src/libqemu-cxx.cc
    src/loader.cc
    src/memory.cc
    src/sysbus.cc
    src/timer.cc
    src/target/aarch64.cc
    src/target/riscv32.cc
    src/target/riscv64.cc
    src/target/microblaze.cc
    ${CMAKE_CURRENT_BINARY_DIR}/src/target_info.cc
)

foreach(target ${LIBQEMU_TARGETS})
    # This variable is used in target_info.cc.in
    get_property(LIBQEMU_TARGET_${target}_LIBRARY
        TARGET libqemu-${target}
        PROPERTY INTERFACE_LINK_LIBRARIES)
endforeach()

configure_file(src/target_info.cc.in src/target_info.cc)

add_library(qemu-cxx SHARED ${srcs})

if(CMAKE_COMPILER_IS_GNUCC)
    target_compile_options(qemu-cxx PRIVATE
        -Wall -Wextra -Wno-unused-parameter
        "$<$<CONFIG:Debug>:-O0;-g3;-ggdb>"
        )
endif()

target_include_directories(qemu-cxx
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(qemu-cxx libqemu)

set_property(TARGET qemu-cxx
    PROPERTY INSTALL_RPATH ${LIBQEMU_LIB_DIR})


install(FILES
    "${LIBQEMU_CXX_SRC_INCLUDE_DIR}/exceptions.h"
    "${LIBQEMU_CXX_SRC_INCLUDE_DIR}/libqemu-cxx.h"
    "${LIBQEMU_CXX_SRC_INCLUDE_DIR}/loader.h"
    "${LIBQEMU_CXX_SRC_INCLUDE_DIR}/target_info.h"
    "${LIBQEMU_CXX_SRC_INCLUDE_DIR}/target/aarch64.h"
    "${LIBQEMU_CXX_SRC_INCLUDE_DIR}/target/riscv32.h"
    "${LIBQEMU_CXX_SRC_INCLUDE_DIR}/target/riscv64.h"
    "${LIBQEMU_CXX_SRC_INCLUDE_DIR}/target/microblaze.h"
    DESTINATION ${LIBQEMU_CXX_INCLUDE_DIR})

if (WIN32)
    install(TARGETS qemu-cxx DESTINATION lib/$<IF:$<CONFIG:Debug>,debug,release>)
else ()
    install(TARGETS qemu-cxx EXPORT libqemu-cxx-targets)
endif ()

install(EXPORT libqemu-cxx-targets DESTINATION ${LIBQEMU_CXX_LIB_DIR}/cmake)

export(EXPORT libqemu-cxx-targets FILE ${CMAKE_CURRENT_BINARY_DIR}/libqemu-cxx-targets.cmake)
export(PACKAGE libqemu-cxx)

include(CMakePackageConfigHelpers)

configure_package_config_file(libqemu-cxxConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/libqemu-cxxConfig.cmake
    INSTALL_DESTINATION ${LIBQEMU_CXX_LIB_DIR}/cmake)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/libqemu-cxxConfigVersion.cmake
    VERSION 1.0.0
    COMPATIBILITY AnyNewerVersion)

install(
    FILES ${CMAKE_BINARY_DIR}/libqemu-cxxConfig.cmake
          ${CMAKE_BINARY_DIR}/libqemu-cxxConfigVersion.cmake
          DESTINATION ${LIBQEMU_CXX_LIB_DIR}/cmake)
