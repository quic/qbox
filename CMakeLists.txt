cmake_minimum_required(VERSION 3.14 FATAL_ERROR)
project(qbox VERSION 2.0)

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

include(FetchContent)
include(CTest)

if(EXISTS "${PROJECT_PATH}/Packages/CPM.cmake/cmake/CPM.cmake")
  include(${PROJECT_PATH}/Packages/CPM.cmake/cmake/CPM.cmake)
else()
  FetchContent_Declare(
    cpm-cmake
    GIT_REPOSITORY https://github.com/cpm-cmake/CPM.cmake.git
    GIT_TAG v0.31.1)
  FetchContent_MakeAvailable(cpm-cmake)
  include(${cpm-cmake_SOURCE_DIR}/cmake/CPM.cmake)
endif()

# Allow using package-lock.cmake files in projects using this boilerplate to pin repos, etc.
set(PKG_LOCK "package-lock.cmake" CACHE STRING "Package lock which will be used by the user")
CPMUsePackageLock(${PKG_LOCK})
# ##############################################

include(${PROJECT_SOURCE_DIR}/cmake/boilerplate.cmake)

if(APPLE)
  set(CMAKE_BUILD_RPATH "\@executable_path/../lib;\@executable_path/../lib/libqemu")
  set(CMAKE_INSTALL_RPATH "\@executable_path/../lib;\@executable_path/../lib/libqemu")
else()
  set(CMAKE_BUILD_RPATH "\$ORIGIN/../lib;\$ORIGIN/../lib/libqemu")
  set(CMAKE_INSTALL_RPATH "\$ORIGIN/../lib;\$ORIGIN/../lib/libqemu")
endif()

set(CMAKE_BUILD_RPATH
    "${CMAKE_BINARY_DIR}/_deps/libqemu-build/qemu-prefix/lib/"
    "${CMAKE_BINARY_DIR}"
)

if ("${CMAKE_INSTALL_PREFIX}" STREQUAL "" OR "${CMAKE_INSTALL_PREFIX}" STREQUAL "/usr/local")
    set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/install)
    message(STATUS "CMAKE PREFIX PATH = ${CMAKE_INSTALL_PREFIX}")
endif()

# This block allow to retrieves the name of the binary directory which is used to ignore SystemC tests
# ----------------------------------------------------------------------------------------
string(REPLACE "${PROJECT_SOURCE_DIR}/" "" CMAKE_BINARY_NAME "${CMAKE_BINARY_DIR}")
message(STATUS "CMAKE_BINARY_NAME = ${CMAKE_BINARY_NAME}")
message(STATUS "PROJECT_SOURCE_DIR = ${PROJECT_SOURCE_DIR}")

configure_file(${PROJECT_SOURCE_DIR}/cmake/CTestCustom.cmake ${CMAKE_BINARY_DIR} @ONLY)
# ----------------------------------------------------------------------------------------

set(CMAKE_CXX_STANDARD
    14
    CACHE STRING "C++ standard to build all targets.")
set(CMAKE_CXX_STANDARD_REQUIRED
    ON
    CACHE BOOL
          "The with CMAKE_CXX_STANDARD selected C++ standard is a requirement.")
mark_as_advanced(CMAKE_CXX_STANDARD_REQUIRED)

include(GNUInstallDirs)

#This is an exclusive define for the SCP library which allow to switch ON/OFF if we want thread local or not.
add_compile_definitions(DISABLE_REPORT_THREAD_LOCAL)

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

gs_addexpackage("gh:nih-at/libzip#v1.10.1")

gs_systemc()

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Other Dependencies
gs_addexpackage("gh:rpclib/rpclib#v2.3.0")

option(WITHOUT_PYTHON_BINDER "without python binder model" OFF)

if((NOT WITHOUT_PYTHON_BINDER) AND (NOT GS_ONLY))
    find_package(Python3 COMPONENTS Interpreter Development NumPy REQUIRED)
    gs_addexpackage("gh:pybind/pybind11#v2.11.1")
    set(PYBIND11_EMBED "pybind11::embed")
    set(PYTHON_BINDER_SRC "systemc-components/python_binder/src/python_binder.cc")
endif()

set(systemc_srcs
    systemc-components/common/src/cciutils.cc
    systemc-components/common/src/luautils.cc
    systemc-components/common/src/uutils.cc
    systemc-components/common/src/memory_services.cc
    systemc-components/common/src/libgssync/pre_suspending_sc_support.cc
    systemc-components/common/src/libgssync/qk_factory.cc
    systemc-components/common/src/libgssync/qkmultithread.cc
    systemc-components/common/src/macs/backends/tap.cc
    systemc-components/common/src/macs/components/mac.cc
    systemc-components/common/src/macs/components/phy.cc
)

if (NOT WITHOUT_QEMU)
    set (qemu_srcs
            qemu-components/common/src/libqemu-cxx/callbacks.cc
            qemu-components/common/src/libqemu-cxx/console.cc
            qemu-components/common/src/libqemu-cxx/cpu.cc
            qemu-components/common/src/libqemu-cxx/device.cc
            qemu-components/common/src/libqemu-cxx/gpex.cc
            qemu-components/common/src/libqemu-cxx/gpio.cc
            qemu-components/common/src/libqemu-cxx/libqemu-cxx.cc
            qemu-components/common/src/libqemu-cxx/loader.cc
            qemu-components/common/src/libqemu-cxx/memory.cc
            qemu-components/common/src/libqemu-cxx/object.cc
            qemu-components/common/src/libqemu-cxx/rcu-read-lock.cc
            qemu-components/common/src/libqemu-cxx/sysbus.cc
            qemu-components/common/src/libqemu-cxx/target-info.cc
            qemu-components/common/src/libqemu-cxx/timer.cc
            qemu-components/common/src/libqemu-cxx/target/aarch64.cc
            qemu-components/common/src/libqemu-cxx/target/riscv.cc
            qemu-components/common/src/libqemu-cxx/target/microblaze.cc
            qemu-components/common/src/libqemu-cxx/target-info.cc
            qemu-components/common/src/dmi_utils.cc
        )
endif()

add_library(${PROJECT_NAME} SHARED
        ${systemc_srcs}
        # qemu_srcs should be empty if WITHOUT_QEMU is set
        ${qemu_srcs}
)

build_lua()

if (NOT WITHOUT_QEMU)
    # This is a hack to workaround a CMake issue. We specify an existing file which
    # is _not_ a submodule so that FetchContent does not init any submodule.
    # See https://gitlab.kitware.com/cmake/cmake/-/issues/20579
    gs_addpackage(qemu/qemu GIT_SUBMODULES "CMakeLists.txt")
    if(TARGET libqemu)
        list(APPEND TARGET_LIBS "libqemu")
    endif()

    set(LIBQEMU_CXX_SRC_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/qemu-components/common/include/libqemu-cxx)
    set(LIBQEMU_CXX_INCLUDE_DIR ${CMAKE_INSTALL_PREFIX})
    set(LIBQEMU_CXX_LIB_DIR ${CMAKE_INSTALL_LIBDIR}/libqemu-cxx)

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

    target_include_directories(
        ${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/qemu-components/common/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}>
    )

    target_link_libraries(${PROJECT_NAME} PUBLIC
        libqemu
    )

    install(DIRECTORY ${LIBQEMU_CXX_SRC_INCLUDE_DIR}
        DESTINATION ${LIBQEMU_CXX_INCLUDE_DIR})

    if (WIN32)
        install(TARGETS ${PROJECT_NAME} DESTINATION lib/$<IF:$<CONFIG:Debug>,debug,release>)
    else ()
        install(TARGETS ${PROJECT_NAME} EXPORT libqemu-cxx-targets)
    endif ()

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

    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/qemu-components" DESTINATION ${CMAKE_INSTALL_PREFIX})
    set(QEMU_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/qemu-components")
   
endif() # WITHOUT_QEMU

if(CMAKE_COMPILER_IS_GNUCC)
        target_compile_options(${PROJECT_NAME} PRIVATE
            -Wall -Wextra -Wno-unused-parameter
            "$<$<CONFIG:Debug>:-O0;-g3;-ggdb>"
            )
endif()

if (APPLE)
    set(CMAKE_MACOSX_RPATH ON)

    # Since dlopen does not automatically look into `brew --prefix`/lib, we
    # add this path to both CMAKE_BUILD_RPATH and CMAKE_INSTALL_RPATH, so
    # the user won't need to set the DYLD_LIBRARY_PATH env variable.
    find_program(BREW_BIN brew)
    execute_process(COMMAND ${BREW_BIN} --prefix
        OUTPUT_STRIP_TRAILING_WHITESPACE
        OUTPUT_VARIABLE BREW_PREFIX)
    set(BREW_LIB ${BREW_PREFIX}/lib)
    list(APPEND CMAKE_BUILD_RPATH ${BREW_LIB})
    list(APPEND CMAKE_INSTALL_RPATH ${BREW_LIB})
endif (APPLE)

# using main branch as it has fixes for cmake
gs_addexpackage("gh:google/googletest#main")

target_include_directories(
    ${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/systemc-components/common/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}>
    ${LIBELF_INCLUDE_DIR}
    )

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

target_link_libraries(${PROJECT_NAME} PUBLIC
        ${TARGET_LIBS}
        ${LIBELF_LIBRARIES}
        Threads::Threads
        rpc
        zip
        ${PYBIND11_EMBED}
        ${CMAKE_DL_LIBS}  
)

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/systemc-components" DESTINATION ${CMAKE_INSTALL_PREFIX})

set(QBOX_INCLUDE_DIR "${QEMU_INCLUDE_DIR};${CMAKE_CURRENT_SOURCE_DIR}/systemc-components")
gs_export(${PROJECT_NAME} ${QBOX_INCLUDE_DIR})

if (NOT DEFINED GS_ONLY)
    install(TARGETS rpc zip DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
endif()
    
list(APPEND TARGET_LIBS ${PROJECT_NAME})

if(NOT WITHOUT_QEMU)
    add_subdirectory(qemu-components)
    add_subdirectory(platforms)
endif()

add_subdirectory(systemc-components)

gs_enable_testing()

if(UNIX AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME} INTERFACE rt)
endif()