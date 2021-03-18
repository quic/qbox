cmake_minimum_required(VERSION 3.14)

project(qbox VERSION 1.0)

##############################################
# ----- fetch GreenSocs cmake BOILERPLATE
##############################################
include(FetchContent)
#FetchContent_Declare(gs-cmake
   #GIT_REPOSITORY https://git.bar.greensocs.com/greensocs/cmake-boilerplate.git
   #GIT_TAG master
#)
#FetchContent_MakeAvailable(gs-cmake)
include(/home/sekoia/work/greensocs/cmake-boilerplate/cmake/gs-boilerplate.cmake)
##############################################

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

gs_systemc()

CPMAddPackage("https://git.bar.greensocs.com/greensocs/libgsutils.git#master")
CPMAddPackage("https://git.bar.greensocs.com/greensocs/libgssync.git#master")
CPMAddPackage("https://git.bar.greensocs.com/qemu/libqemu-cxx.git#master")
CPMAddPackage("https://git.bar.greensocs.com/greensocs/components/base-components.git#master")

set(LIBQBOX_LIB_DIR ${CMAKE_INSTALL_LIBDIR}/libqbox)

add_library(${PROJECT_NAME} INTERFACE)

target_include_directories(${PROJECT_NAME}
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_link_libraries(${PROJECT_NAME} INTERFACE
    SystemC::systemc SystemC::cci
    qemu-cxx
    GreenSocs::libgsutils GreenSocs::libgssync
    GreenSocs::base-components)

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/libqbox" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

gs_export()
