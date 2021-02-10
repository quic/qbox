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
)

foreach(target ${LIBQEMU_TARGETS})
    list(APPEND srcs target/${target}.cc)
endforeach()

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
    "${CMAKE_CURRENT_SOURCE_DIR}/libqemu-cxx.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/exceptions.h"
    DESTINATION include/libqemu-cxx)

foreach(target ${LIBQEMU_TARGETS})
    install(FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/target/${target}.h"
        DESTINATION include/libqemu-cxx/target)
endforeach()

if (WIN32)
    install(TARGETS qemu-cxx DESTINATION lib/$<IF:$<CONFIG:Debug>,debug,release>)
else ()
    install(TARGETS qemu-cxx DESTINATION lib)
endif ()
