cmake_minimum_required(VERSION 3.3)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

project(libqbox)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(SystemC REQUIRED COMPONENTS CCI)
find_package(libqemu-cxx CONFIG REQUIRED)
find_package(libgs REQUIRED)

set(LIBQBOX_LIB_DIR ${CMAKE_INSTALL_LIBDIR}/libqbox)

add_library(qbox SHARED)

if(MSVC)
    target_compile_options(qbox PRIVATE /W4)
else()
    target_compile_options(qbox PRIVATE
        -Wall -Wextra
        "$<$<CONFIG:Debug>:-O0;-ggdb3>")
endif()

target_include_directories(qbox
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${LIBGS_INCLUDE_DIR})

target_link_libraries(qbox
    SystemC::SystemC
    qemu-cxx
    ${LIBGS_LIBRARIES})

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/libqbox" DESTINATION include)
install(TARGETS qbox DESTINATION lib EXPORT libqbox-targets)
install(EXPORT libqbox-targets DESTINATION ${LIBQBOX_LIB_DIR}/cmake)

export(EXPORT libqbox-targets)
export(PACKAGE libqbox)

configure_package_config_file(libqboxConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/libqboxConfig.cmake
    INSTALL_DESTINATION ${LIBQBOX_LIB_DIR}/cmake)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/libqboxConfigVersion.cmake
    VERSION 1.0.0
    COMPATIBILITY AnyNewerVersion)

install(
    FILES ${CMAKE_BINARY_DIR}/libqboxConfig.cmake
          ${CMAKE_BINARY_DIR}/libqboxConfigVersion.cmake
    DESTINATION ${LIBQBOX_LIB_DIR}/cmake)
