cmake_minimum_required(VERSION 3.14)
project(base-components VERSION 1.0)

##############################################
# ----- fetch GreenSocs cmake BOILERPLATE
##############################################
if (NOT gs-cmake_SOURCE_DIR)
   include(FetchContent)
   FetchContent_Declare(gs-cmake
      GIT_REPOSITORY git@git.greensocs.com:greensocs/cmake-boilerplate.git
      GIT_TAG master
   )
   FetchContent_MakeAvailable(gs-cmake)
endif()
include(${gs-cmake_SOURCE_DIR}/cmake/gs-boilerplate.cmake)
##############################################

find_path(LIBELF_INCLUDE_DIRS NAMES "libelf.h"
          HINTS $ENV{LIBELF_HOME}/include /usr/include /usr/local/include/libelf)

find_library(LIBELF_LIBRARIES NAMES elf "libelf.a"
          HINTS $ENV{LIBELF_HOME}/lib /usr/lib /lib /usr/local/lib)

find_library(LIBZ_LIBRARIES NAMES z "libz.a"
          HINTS $ENV{LIBZ_HOME}/lib /usr/lib /lib)

list(APPEND LIBELF_LIBRARIES ${LIBZ_LIBRARIES})
list(APPEND LIBELF_INCLUDE_DIRS "/usr/local/include")

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LibELF DEFAULT_MSG
                                  LIBELF_LIBRARIES
                                  LIBELF_INCLUDE_DIRS)

mark_as_advanced(LIBELF_INCLUDE_DIRS LIBELF_LIBRARIES)

gs_systemc()

gs_addpackage(greensocs/libgsutils)

add_library(${PROJECT_NAME} INTERFACE)

target_include_directories(${PROJECT_NAME}
    INTERFACE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        ${LIBELF_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}
    INTERFACE ${TARGET_LIBS}
    INTERFACE ${LIBELF_LIBRARIES}
)
gs_export()

gs_enable_testing()
