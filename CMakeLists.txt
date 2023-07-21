cmake_minimum_required(VERSION 3.14)
project(qbox  VERSION 1.0)

##############################################
# ----- fetch GreenSocs cmake BOILERPLATE
##############################################
set(GREENSOCS_GIT "" CACHE STRING "Git directory from which to clone all gs repositories")

execute_process(COMMAND git config --get remote.origin.url
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_STRIP_TRAILING_WHITESPACE
    OUTPUT_VARIABLE GIT_URL)

string(REGEX MATCH "(.*\/)" GREENSOCS_GIT_ORIGIN "${GIT_URL}")
string(REGEX REPLACE "(.*)/sandboxes/[^\/]*/(.*)" "\\1/" GREENSOCS_GIT_ORIGIN "${GREENSOCS_GIT_ORIGIN}")

if ("${GREENSOCS_GIT}" STREQUAL "")
    set(GREENSOCS_GIT "${GREENSOCS_GIT_ORIGIN}")
endif()

if (NOT gs-cmake_SOURCE_DIR)
   include(FetchContent)
   FetchContent_Declare(gs-cmake
        GIT_REPOSITORY ${GREENSOCS_GIT}cmake-boilerplate.git
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
          HINTS $ENV{LIBELF_HOME}/lib /usr/lib /lib /usr/local/lib
          REQUIRED)

find_library(LIBZ_LIBRARIES NAMES z "libz.a"
          HINTS $ENV{LIBZ_HOME}/lib /usr/lib /lib
          REQUIRED)

list(APPEND LIBELF_LIBRARIES ${LIBZ_LIBRARIES})
list(APPEND LIBELF_INCLUDE_DIR ${LIBELF_INCLUDE_DIR}/libelf)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LibELF DEFAULT_MSG
                                  LIBELF_LIBRARIES
                                  LIBELF_INCLUDE_DIR)

mark_as_advanced(LIBELF_INCLUDE_DIR LIBELF_LIBRARIES)

find_package(Threads REQUIRED)

gs_systemc()

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# This is a hack to workaround a CMake issue. We specify an existing file which
# is _not_ a submodule so that FetchContent does not init any submodule.
# See https://gitlab.kitware.com/cmake/cmake/-/issues/20579
gs_addpackage(qemu/qemu GIT_SUBMODULES "CMakeLists.txt")

set(LIBQEMU_CXX_SRC_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include/libqemu-cxx)
set(LIBQEMU_CXX_INCLUDE_DIR ${CMAKE_INSTALL_INCLUDEDIR})
set(LIBQEMU_CXX_LIB_DIR ${CMAKE_INSTALL_LIBDIR}/libqemu-cxx)

# Other Dependencies
gs_addexpackage("gh:rpclib/rpclib#v2.3.0")

set(srcs
    src/libqemu-cxx/callbacks.cc
    src/libqemu-cxx/console.cc
    src/libqemu-cxx/cpu.cc
    src/libqemu-cxx/device.cc
    src/libqemu-cxx/gpex.cc
    src/libqemu-cxx/gpio.cc
    src/libqemu-cxx/libqemu-cxx.cc
    src/libqemu-cxx/loader.cc
    src/libqemu-cxx/memory.cc
    src/libqemu-cxx/object.cc
    src/libqemu-cxx/rcu-read-lock.cc
    src/libqemu-cxx/sysbus.cc
    src/libqemu-cxx/timer.cc
    src/libqemu-cxx/target/aarch64.cc
    src/libqemu-cxx/target/riscv.cc
    src/libqemu-cxx/target/microblaze.cc
    src/libqemu-cxx/target-info.cc
)

add_library(${PROJECT_NAME} SHARED
   src/libgssync/qkmultithread.cc
   src/libgssync/qk_factory.cc
   src/libgssync/pre_suspending_sc_support.cc
   src/systemc-macs/backends/tap.cc
   src/systemc-macs/models/dwmac.cc
   src/systemc-macs/components/mac.cc
   src/systemc-macs/components/phy.cc
   src/systemc-macs/models/xgmac.cc
   src/libqbox-extra/components/pci/virtio-gpu-gl-pci.cc
   ${srcs}
)

gs_addexpackage("gh:lua/lua#v5.4.2")
if (lua_ADDED)
  # lua has no CMake support, so we create our own target
  FILE(GLOB lua_sources ${lua_SOURCE_DIR}/*.c)
  list(REMOVE_ITEM lua_sources "${lua_SOURCE_DIR}/lua.c" "${lua_SOURCE_DIR}/luac.c" "${lua_SOURCE_DIR}/ltests.c" "${lua_SOURCE_DIR}/onelua.c")
  add_library(lua STATIC ${lua_sources})
  target_compile_definitions(lua PRIVATE LUA_USE_POSIX)
  target_include_directories(lua PUBLIC $<BUILD_INTERFACE:${lua_SOURCE_DIR}>)
  target_compile_definitions(${PROJECT_NAME} INTERFACE HAS_LUA)
  target_link_libraries(${PROJECT_NAME} INTERFACE lua)
  install(TARGETS lua EXPORT ${PROJECT_NAME}Targets)
endif()

foreach(target ${LIBQEMU_TARGETS})
    # The quite complex generator expressions does the followings:
    #   - fetch the library name through the INTERFACE_LINK_LIBRARIES property
    #   - evaluate it as a generator expression since it can contains
    #     $<BUILD_INTERFACE> and $<INSTALL_INTERFACE> in a list
    #   - join the result to remove empty elements in the final list
    target_compile_definitions(${PROJECT_NAME}
        PRIVATE
            LIBQEMU_TARGET_${target}_LIBRARY="$<JOIN:$<GENEX_EVAL:$<TARGET_PROPERTY:libqemu-${target},INTERFACE_LINK_LIBRARIES>>,>")
endforeach()

if(CMAKE_COMPILER_IS_GNUCC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall -Wextra -Wno-unused-parameter
        "$<$<CONFIG:Debug>:-O0;-g3;-ggdb>"
        )
endif()

if (APPLE)
    set(CMAKE_MACOSX_RPATH ON)
endif (APPLE)
# using main branch as it has fixes for cmake
gs_addexpackage("gh:google/googletest#main")

find_package(SDL2 REQUIRED)

target_include_directories(
   ${PROJECT_NAME} PUBLIC
   $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
   $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
   ${LIBELF_INCLUDE_DIR}
   ${SDL2_INCLUDE_DIRS}
   PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

target_link_libraries(${PROJECT_NAME} PUBLIC
     ${TARGET_LIBS}
     ${LIBELF_LIBRARIES}
     Threads::Threads
     ${SDL2_LIBRARIES}
     libqemu
     rpc
     ${CMAKE_DL_LIBS}
)

install(DIRECTORY ${LIBQEMU_CXX_SRC_INCLUDE_DIR}
    DESTINATION ${LIBQEMU_CXX_INCLUDE_DIR})

if (WIN32)
    install(TARGETS ${PROJECT_NAME} DESTINATION lib/$<IF:$<CONFIG:Debug>,debug,release>)
else ()
    install(TARGETS ${PROJECT_NAME} EXPORT libqemu-cxx-targets)
endif ()

# install(EXPORT libqemu-cxx-targets DESTINATION ${LIBQEMU_CXX_LIB_DIR}/cmake)

# export(EXPORT libqemu-cxx-targets FILE ${CMAKE_CURRENT_BINARY_DIR}/libqemu-cxx-targets.cmake)
# export(PACKAGE qbox)

include(CMakePackageConfigHelpers)

configure_package_config_file(libqemu-cxxConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/libqemu-cxxConfig.cmake
    INSTALL_DESTINATION ${LIBQEMU_CXX_LIB_DIR}/cmake)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/libqemu-cxxConfigVersion.cmake
    VERSION 1.0.0
    COMPATIBILITY AnyNewerVersion)

install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/libqemu-cxxConfig.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/libqemu-cxxConfigVersion.cmake
            DESTINATION ${LIBQEMU_CXX_LIB_DIR}/cmake)

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/libqbox-extra" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/libqbox" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

if(UNIX AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME} INTERFACE rt)
endif()

gs_export()
install(TARGETS rpc DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)

add_subdirectory(platforms)

gs_enable_testing()
