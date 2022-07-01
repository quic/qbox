cmake_minimum_required(VERSION 3.14)

project(qbox VERSION 1.0)

##############################################
# ----- fetch GreenSocs cmake BOILERPLATE
##############################################
if (NOT gs-cmake_SOURCE_DIR)
   include(FetchContent)
   FetchContent_Declare(gs-cmake
      GIT_REPOSITORY git@gitlab.qualcomm.com:qqvp/greensocs/components/cmake-boilerplate.git
      GIT_TAG master
   )
   FetchContent_MakeAvailable(gs-cmake)
endif()
include(${gs-cmake_SOURCE_DIR}/cmake/gs-boilerplate.cmake)
##############################################

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

gs_systemc()

gs_addpackage(greensocs/libgsutils)
gs_addpackage(greensocs/libgssync)
gs_addpackage(qemu/libqemu-cxx)
gs_addpackage(greensocs/components/base-components)

set(LIBQBOX_LIB_DIR ${CMAKE_INSTALL_LIBDIR}/libqbox)

add_library(${PROJECT_NAME} INTERFACE)

target_include_directories(${PROJECT_NAME}
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_link_libraries(${PROJECT_NAME} INTERFACE
    ${TARGET_LIBS}
    qemu-cxx
    )

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/libqbox" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

gs_export()

gs_enable_testing()
