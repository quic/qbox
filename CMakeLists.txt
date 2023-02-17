cmake_minimum_required(VERSION 3.14)
project(base-components VERSION 1.0)

##############################################
# ----- fetch GreenSocs cmake BOILERPLATE
##############################################
set(PACKAGE_PATH "greensocs/components/base-components")
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

find_path(LIBELF_INCLUDE_DIR NAMES "libelf.h" "libelf/libelf.h"
          HINTS $ENV{LIBELF_HOME}/include /usr/include /usr/local/include
          REQUIRED)

find_library(LIBELF_LIBRARIES NAMES elf "libelf.a"
          HINTS $ENV{LIBELF_HOME}/lib /usr/lib /lib /usr/local/lib)

find_library(LIBZ_LIBRARIES NAMES z "libz.a"
          HINTS $ENV{LIBZ_HOME}/lib /usr/lib /lib)

list(APPEND LIBELF_LIBRARIES ${LIBZ_LIBRARIES})
list(APPEND LIBELF_INCLUDE_DIR ${LIBELF_INCLUDE_DIR}/libelf)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LibELF DEFAULT_MSG
                                  LIBELF_LIBRARIES
                                  LIBELF_INCLUDE_DIR)

mark_as_advanced(LIBELF_INCLUDE_DIR LIBELF_LIBRARIES)

find_package(Threads REQUIRED)

gs_systemc()

gs_addpackage(greensocs/libgsutils)
gs_addpackage(greensocs/libgssync)
gs_addexpackage("gh:rpclib/rpclib#v2.3.0")

add_library(${PROJECT_NAME} INTERFACE)

target_include_directories(${PROJECT_NAME}
    INTERFACE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        ${LIBELF_INCLUDE_DIR}
)

target_link_libraries(${PROJECT_NAME}
    INTERFACE ${TARGET_LIBS}
    INTERFACE ${LIBELF_LIBRARIES}
    INTERFACE Threads::Threads
    INTERFACE rpc
)

if(UNIX AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME} INTERFACE rt)
endif()

gs_export()

gs_enable_testing()
