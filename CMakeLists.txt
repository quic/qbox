cmake_minimum_required(VERSION 3.14)
project(greensocs-base-vp)

##############################################
# ----- fetch GreenSocs cmake BOILERPLATE
##############################################
if(EXISTS "${PROJECT_SOURCE_DIR}/Packages" AND EXISTS "${PROJECT_SOURCE_DIR}/Packages/cmake-boilerplate")
    set (gs-cmake_SOURCE_DIR  "${PROJECT_SOURCE_DIR}/Packages/cmake-boilerplate")
endif()
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

gs_systemc()

set(LIBQEMU_TARGETS aarch64 hexagon)
gs_addpackage(qemu/libqbox)
gs_addpackage(greensocs/components/base-components)
gs_addpackage(greensocs/components/systemc-uarts)

gs_addpackage(qemu/extra-components)
gs_addpackage(customers/qualcomm/ipcc)
gs_addpackage(customers/qualcomm/qtb)

add_executable(vp
    src/main.cc
)

target_link_libraries(vp
    SystemC::systemc
    SystemC::cci
    GreenSocs::qbox
    GreenSocs::qbox-extra
    GreenSocs::base-components
    GreenSocs::systemc-uarts
    GreenSocs::qcom-ipcc
    GreenSocs::qcom-qtb
)

install (TARGETS vp DESTINATION ${CMAKE_INSTALL_BINDIR})

gs_enable_testing()
