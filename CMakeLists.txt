cmake_minimum_required(VERSION 3.14)
project(greensocs-base-vp)

##############################################
# ----- fetch GreenSocs cmake BOILERPLATE
##############################################
if(EXISTS "${PROJECT_SOURCE_DIR}/Packages" AND EXISTS "${PROJECT_SOURCE_DIR}/Packages/cmake-boilerplate")
    set (gs-cmake_SOURCE_DIR  "${PROJECT_SOURCE_DIR}/Packages/cmake-boilerplate")
endif()

set(PACKAGE_PATH "quic-vp")
set(GREENSOCS_GIT "" CACHE STRING "Git directory from which to clone all gs repositoies")

execute_process(COMMAND git config --get remote.origin.url
    OUTPUT_STRIP_TRAILING_WHITESPACE
    OUTPUT_VARIABLE GIT_URL)

string(REGEX REPLACE "(.*)/${PACKAGE_PATH}.git$" "\\1/" GREENSOCS_GIT_ORIGIN "${GIT_URL}")
string(REGEX REPLACE "(.*)/sandboxes/[^\/]*/(.*)" "\\1/" GREENSOCS_GIT_ORIGIN "${GREENSOCS_GIT_ORIGIN}")

if ( "${GREENSOCS_GIT}" STREQUAL "")
    set(GREENSOCS_GIT "${GREENSOCS_GIT_ORIGIN}")
endif()

if (NOT gs-cmake_SOURCE_DIR)
   include(FetchContent)
   FetchContent_Declare(gs-cmake
        GIT_REPOSITORY ${GREENSOCS_GIT}greensocs/cmake-boilerplate.git
        GIT_TAG master
   )
   FetchContent_MakeAvailable(gs-cmake)
endif()
include(${gs-cmake_SOURCE_DIR}/cmake/gs-boilerplate.cmake)

##############################################

set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

gs_systemc()

set(LIBQEMU_TARGETS aarch64 hexagon)
gs_addpackage(qemu/libqbox)
gs_addpackage(greensocs/components/base-components)
gs_addpackage(greensocs/components/systemc-uarts)

gs_addpackage(qemu/extra-components)
gs_addpackage(quic/components/ipcc)
gs_addpackage(quic/components/qtb)
gs_addpackage(quic/components/csr)

add_executable(vp
    src/main.cc
)
if(APPLE)
    target_link_options(vp PRIVATE "LINKER:-dead_strip_dylibs")
else()
    target_link_options(vp PRIVATE "LINKER:-as-needed")
endif()

target_link_libraries(vp
    ${TARGET_LIBS}
)

install (TARGETS vp DESTINATION ${CMAKE_INSTALL_BINDIR})

gs_enable_testing()
