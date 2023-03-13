cmake_minimum_required(VERSION 3.14)

project(qbox-extra VERSION 1.0)

##############################################
# ----- fetch GreenSocs cmake BOILERPLATE
##############################################
set(PACKAGE_PATH "qemu/extra-components")
set(GREENSOCS_GIT "" CACHE STRING "Git directory from which to clone all gs repositoies")

execute_process(COMMAND git config --get remote.origin.url
    OUTPUT_STRIP_TRAILING_WHITESPACE
    OUTPUT_VARIABLE GIT_URL
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

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

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

gs_addpackage(qemu/libqbox)

set(LIBQBOX_EXTRA_LIB_DIR ${CMAKE_INSTALL_LIBDIR}/libqbox-extra)

add_library(${PROJECT_NAME} INTERFACE)

target_include_directories(${PROJECT_NAME}
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_link_libraries(${PROJECT_NAME} INTERFACE ${TARGET_LIBS})

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/libqbox-extra" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

gs_export()