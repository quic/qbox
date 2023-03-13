cmake_minimum_required(VERSION 3.14)
project(systemc-uarts VERSION 1.0)

##############################################
# ----- fetch GreenSocs cmake BOILERPLATE
##############################################
set(PACKAGE_PATH "greensocs/components/systemc-uarts")
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

gs_systemc()

gs_addpackage(greensocs/libgsutils)
gs_addpackage(greensocs/libgssync)

add_library(${PROJECT_NAME} INTERFACE)

target_include_directories(
    ${PROJECT_NAME} INTERFACE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(${PROJECT_NAME} INTERFACE
    ${TARGET_LIBS}
)

gs_export()
gs_enable_testing()
