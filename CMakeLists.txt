cmake_minimum_required (VERSION 3.1)

project(libqemu-cxx)

##############################################
# ----- fetch GreenSocs cmake BOILERPLATE
##############################################
if (NOT gs-cmake_SOURCE_DIR)
   include(FetchContent)
   FetchContent_Declare(gs-cmake
      GIT_REPOSITORY https://git.greensocs.com/greensocs/cmake-boilerplate.git
      GIT_TAG master
   )
   FetchContent_MakeAvailable(gs-cmake)
endif()
include(${gs-cmake_SOURCE_DIR}/cmake/gs-boilerplate.cmake)
##############################################

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# This is a hack to workaround a CMake issue. We specify an existing file which
# is _not_ a submodule so that FetchContent does not init any submodule.
# See https://gitlab.kitware.com/cmake/cmake/-/issues/20579
gs_addpackage(qemu/qemu GIT_SUBMODULES "CMakeLists.txt")

set(LIBQEMU_CXX_SRC_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include/libqemu-cxx)
set(LIBQEMU_CXX_INCLUDE_DIR ${CMAKE_INSTALL_INCLUDEDIR})
set(LIBQEMU_CXX_LIB_DIR ${CMAKE_INSTALL_LIBDIR}/libqemu-cxx)

set(srcs
    src/callbacks.cc
    src/cpu.cc
    src/device.cc
    src/gpex.cc
    src/gpio.cc
    src/libqemu-cxx.cc
    src/loader.cc
    src/memory.cc
    src/object.cc
    src/sysbus.cc
    src/timer.cc
    src/target/aarch64.cc
    src/target/riscv.cc
    src/target/microblaze.cc
    src/target-info.cc
)


add_library(qemu-cxx SHARED ${srcs})

foreach(target ${LIBQEMU_TARGETS})
    # The quite complex generator expressions does the followings:
    #   - fetch the library name through the INTERFACE_LINK_LIBRARIES property
    #   - evaluate it as a generator expression since it can contains
    #     $<BUILD_INTERFACE> and $<INSTALL_INTERFACE> in a list
    #   - join the result to remove empty elements in the final list
    target_compile_definitions(qemu-cxx
        PRIVATE
            LIBQEMU_TARGET_${target}_LIBRARY="$<JOIN:$<GENEX_EVAL:$<TARGET_PROPERTY:libqemu-${target},INTERFACE_LINK_LIBRARIES>>,>")
endforeach()

if(CMAKE_COMPILER_IS_GNUCC)
    target_compile_options(qemu-cxx PRIVATE
        -Wall -Wextra -Wno-unused-parameter
        "$<$<CONFIG:Debug>:-O0;-g3;-ggdb>"
        )
endif()

target_include_directories(qemu-cxx
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${LIBQEMU_CXX_INCLUDE_DIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src

)

target_link_libraries(qemu-cxx libqemu ${CMAKE_DL_LIBS})

set_property(TARGET qemu-cxx
    PROPERTY INSTALL_RPATH ${LIBQEMU_LIB_DIR})


install(DIRECTORY ${LIBQEMU_CXX_SRC_INCLUDE_DIR}
    DESTINATION ${LIBQEMU_CXX_INCLUDE_DIR})

if (WIN32)
    install(TARGETS qemu-cxx DESTINATION lib/$<IF:$<CONFIG:Debug>,debug,release>)
else ()
    install(TARGETS qemu-cxx EXPORT libqemu-cxx-targets)
endif ()

install(EXPORT libqemu-cxx-targets DESTINATION ${LIBQEMU_CXX_LIB_DIR}/cmake)

export(EXPORT libqemu-cxx-targets FILE ${CMAKE_CURRENT_BINARY_DIR}/libqemu-cxx-targets.cmake)
export(PACKAGE libqemu-cxx)

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
