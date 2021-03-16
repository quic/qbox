cmake_minimum_required(VERSION 3.14)
project(base-components VERSION 1.0)

##############################################
# ----- fetch GreenSocs cmake BOILERPLATE
##############################################
include(FetchContent)
FetchContent_Declare(gs-cmake
   GIT_REPOSITORY https://git.bar.greensocs.com/greensocs/cmake-boilerplate.git
   GIT_TAG master
)
FetchContent_MakeAvailable(gs-cmake)
include(${gs-cmake_SOURCE_DIR}/cmake/gs-boilerplate.cmake)
##############################################

gs_systemc()

CPMAddPackage("https://git.bar.greensocs.com/greensocs/libgsutils.git#master")

add_library(${PROJECT_NAME} INTERFACE)

target_include_directories(${PROJECT_NAME}
    INTERFACE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(${PROJECT_NAME}
    INTERFACE SystemC::systemc SystemC::cci)
gs_export()
