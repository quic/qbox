cmake_minimum_required(VERSION 3.14 FATAL_ERROR)
project(qbox VERSION 2.0)

##############################################
# ----- fetch GreenSocs cmake BOILERPLATE
##############################################

# Note that GS_ENABLE_AUTO_INIT requires clang >= 8; gcc >= 12
option(GS_ENABLE_AUTO_INIT "Enable trivial auto variable initialization" OFF)
option(GS_ENABLE_CXXLIB_CHECK "Enable C, C++ library checks" OFF)
option(GS_ENABLE_GLIBC "Use glibc instead of some other host C library" ON)
option(GS_ENABLE_LIBCXX "Use libc++ instead of libstdc++" OFF)
option(GS_ENABLE_LLD "Link using lld instead of default (ld.bfd)" OFF)
option(GS_ENABLE_LTO "Use LTO to build" OFF)
option(GS_ENABLE_SANITIZERS "Enable ASan+UBSan" OFF)
option(GS_ENABLE_SCUDO "Enable Scudo allocator" OFF)
option(GS_ENABLE_TSAN "Enable Thread Sanitizer" OFF)
option(GS_ENABLE_VIRCLRENDERER "Enable/Disable virclrenderer" ON)
option(GS_ENABLE_VIRGLRENDERER "Enable/Disable virglrenderer" ON)
option(WITHOUT_PYTHON_BINDER "without python binder model" OFF)
option(QEMU_ENABLE_USB_REDIRECT "enable qemu usb redirect" OFF)

set(CMAKE_CXX_STANDARD 14 CACHE STRING "C++ standard to build all targets.")
set(CMAKE_CXX_STANDARD_REQUIRED ON CACHE BOOL "The with CMAKE_CXX_STANDARD selected C++ standard is a requirement.")
set(GIT_BRANCH "main" CACHE STRING "Git branch from which to clone all gs repositoies")
set(GREENSOCS_GIT "" CACHE STRING "Git directory from which to clone all gs repositories")
set(PKG_LOCK "package-lock.cmake" CACHE STRING "Package lock which will be used by the user")
set(QEMU_PATH_NAME "" CACHE STRING "The name of the path of qemu in the git directory")

set(CMAKE_FIND_USE_PACKAGE_REGISTRY FALSE)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)

# Generate a compile_commands.json file for our build,
# for use by clang_complete, YouCompleteMe, vscode, etc.
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

include(CMakePackageConfigHelpers)
include(CTest)
include(GNUInstallDirs)
include(FetchContent)
include(FindPackageHandleStandardArgs)

file(
    DOWNLOAD
    https://github.com/cpm-cmake/CPM.cmake/releases/download/v0.40.5/CPM.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake
)
include(${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/boilerplate.cmake)

macro(determine_qemu_git_path)
    string(FIND "${GREENSOCS_GIT}" "github" GITHUB_URL)

    if(GITHUB_URL EQUAL -1)
        set(QEMU_PATH_NAME "qemu/qemu")
    else()
        set(QEMU_PATH_NAME "qemu")
    endif()

    message(STATUS "QEMU_PATH_NAME=${QEMU_PATH_NAME}")
endmacro()

macro(determine_qbox_git_url)
    execute_process(
        COMMAND git config --get remote.origin.url
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_STRIP_TRAILING_WHITESPACE
        OUTPUT_VARIABLE GIT_URL
    )

    string(REGEX MATCH "(.*\/)" GREENSOCS_GIT_ORIGIN "${GIT_URL}")
    string(
        REGEX
        REPLACE "(.*)/sandboxes/[^\/]*/(.*)" "\\1/"
        GREENSOCS_GIT_ORIGIN "${GREENSOCS_GIT_ORIGIN}"
    )

    if("${GREENSOCS_GIT}" STREQUAL "")
        set(GREENSOCS_GIT "${GREENSOCS_GIT_ORIGIN}")
    endif()

    determine_qemu_git_path()

    if(EXISTS "${PROJECT_SOURCE_DIR}/Packages")
        if("${GREENSOCS_GIT}" STREQUAL "")
            set(GREENSOCS_GIT "https://git.codelinaro.org/clo/private/qqvp/")
        endif()
    endif()

    message(STATUS "GREENSOCS_GIT = ${GREENSOCS_GIT}")
endmacro()

determine_qbox_git_url()

# Allow using package-lock.cmake files in projects using this boilerplate to
# pin repos, etc.
CPMUsePackageLock(${PKG_LOCK})

# ##############################################
if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
    message(FATAL_ERROR "Please use a build directory.")
endif()

# Only include/use/update a pre-existing package cache. Otherwise, a package
# cache can be built by including it on the command line with -DCPM_SOURCE_CACHE
if(EXISTS "${PROJECT_SOURCE_DIR}/Packages")
    message(STATUS "Using Packages cache")
    set(ENV{CPM_SOURCE_CACHE} "${PROJECT_SOURCE_DIR}/Packages")
endif()

# ######################## COMPILATIONS OPTIONS ################################
if(APPLE)
    message(STATUS "Adding flag -undefined dynamic_lookup to build dependent dylib's")
    set(
        CMAKE_SHARED_LIBRARY_CREATE_CXX_FLAGS
        "${CMAKE_SHARED_LIBRARY_CREATE_CXX_FLAGS} -undefined dynamic_lookup"
    )
endif()

if(GS_ENABLE_LTO)
    message(STATUS "LTO build enabled")
    add_compile_options("-flto")
    add_link_options("-flto")

    set(GS_ENABLE_LLD ON CACHE BOOL "" FORCE)
endif()

if(GS_ENABLE_LLD)
    add_link_options("-fuse-ld=lld")
endif()

if(GS_ENABLE_SCUDO)
    message(STATUS "Scudo allocator enabled")
    add_compile_options("-fsanitize=scudo")
    add_link_options("-fsanitize=scudo")
endif()

if(GS_ENABLE_SANITIZERS)
    message(STATUS "ASan/UBSan enabled")
    add_compile_options("-fsanitize=address,undefined")
    add_link_options("-fsanitize=address,undefined")
    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        add_link_options("-shared-libasan")
    endif()
endif()

if(GS_ENABLE_TSAN)
    message(STATUS "TSan enabled")
    add_compile_options("-fsanitize=thread")
    add_link_options("-fsanitize=thread")
endif()

if(GS_ENABLE_LIBCXX)
    message(STATUS "libc++ enabled")
    add_compile_options("-stdlib=libc++")
    add_link_options("-stdlib=libc++")
endif()

if(GS_ENABLE_CXXLIB_CHECK)
    message(STATUS "C/C++ library checks enabled")
    if (GS_ENABLE_GLIBC)
        add_compile_definitions("_FORTIFY_SOURCE=1")
    endif()
    if (NOT GS_ENABLE_LIBCXX)
        set(cxx_defs _GLIBCXX_DEBUG=1 _GLIBCXX_CONCEPT_CHECKS=1)
    endif()
    add_compile_definitions("$<$<COMPILE_LANGUAGE:CXX>:${cxx_defs}>")
endif()

if(GS_ENABLE_AUTO_INIT)
    message(STATUS "auto variable init pattern enabled")
    add_compile_options("-ftrivial-auto-var-init=pattern")
endif()
# ##############################################################################

if(APPLE)
  set(CMAKE_BUILD_RPATH "\@executable_path/../lib;\@executable_path/../lib/libqemu")
  set(CMAKE_INSTALL_RPATH "\@executable_path/../lib;\@executable_path/../lib/libqemu")
else()
  set(CMAKE_BUILD_RPATH "\$ORIGIN/../lib;\$ORIGIN/../lib/libqemu")
  set(CMAKE_INSTALL_RPATH "\$ORIGIN/../lib;\$ORIGIN/../lib/libqemu")
endif()

list(APPEND CMAKE_BUILD_RPATH
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

mark_as_advanced(CMAKE_CXX_STANDARD_REQUIRED)

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

FIND_PACKAGE_HANDLE_STANDARD_ARGS(LibELF DEFAULT_MSG
                                  LIBELF_LIBRARIES
                                  LIBELF_INCLUDE_DIR)

mark_as_advanced(LIBELF_INCLUDE_DIR LIBELF_LIBRARIES)

find_package(Threads REQUIRED)

gs_addexpackage("gh:google/googletest#v1.15.2")
gs_addexpackage("gh:lua/lua#v5.4.2")
gs_addexpackage("gh:nih-at/libzip#v1.10.1")
gs_addexpackage("gh:rpclib/rpclib#v2.3.0")
gs_addexpackage("gh:TheLartians/PackageProject.cmake@1.4.1")

gs_systemc()

# libasio-dev should be installed on the system
if(NOT GS_ONLY)
    set(CROW_BUILD_EXAMPLES OFF CACHE STRING "Disable building Crow examples")
    set(DCROW_BUILD_TESTS OFF CACHE STRING "Disable building Crow tests")
    set(CROW_INSTALL ON CACHE STRING "Enable Crow install")
    gs_addexpackage("gh:CrowCpp/Crow#v1.2.1.2")
    set(CROW_DEP "Crow::Crow")
endif()

if((NOT WITHOUT_PYTHON_BINDER) AND (NOT GS_ONLY))
    find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
    set(PYBIND11_FINDPYTHON ON)
    gs_addexpackage("gh:pybind/pybind11#v3.0.1")
    set(PYBIND11_EMBED "pybind11::embed")
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
            qemu-components/common/src/libqemu-cxx/target/hexagon.cc
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
    # TODO: Remove the "hack" in the future, once support for Ubuntu 20 can be
    # dropped. A fix for it was added in CMake 3.18, but the default version of
    # Ubuntu 20 is 3.16
    # This is a hack to workaround a CMake issue. We specify an existing file
    # which is _not_ a submodule so that FetchContent does not init any
    # submodule. See https://gitlab.kitware.com/cmake/cmake/-/issues/20579
    CPMAddPackage(
        NAME qemu
        GIT_REPOSITORY "${GREENSOCS_GIT}/${QEMU_PATH_NAME}.git"
        GIT_TAG "${GIT_BRANCH}"
        GIT_SHALLOW TRUE
        GIT_SUBMODULES "CMakeLists.txt"
    )

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

target_include_directories(
    ${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/systemc-components/common/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}>
    ${LIBELF_INCLUDE_DIR}
    )

target_link_libraries(${PROJECT_NAME} PUBLIC
        ${TARGET_LIBS}
        ${LIBELF_LIBRARIES}
        Threads::Threads
        rpc
        ${CROW_DEP}
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
if((NOT WITHOUT_PYTHON_BINDER) AND (NOT GS_ONLY))
    add_subdirectory(py-models)
endif()
add_subdirectory(html)

gs_enable_testing()

if(UNIX AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME} INTERFACE rt)
endif()
