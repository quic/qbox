cmake_minimum_required(VERSION 3.3)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

project(libqbox)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if (WIN32)
    find_package(SystemC REQUIRED)
else ()
    find_package(SystemC REQUIRED COMPONENTS CCI)
endif ()

find_package(PkgConfig REQUIRED)
if (PkgConfig_FOUND)
    pkg_check_modules(slirp slirp)
endif()

find_package(libqemu-cxx CONFIG REQUIRED)

add_library(qbox SHARED
    src/libqbox.cc
    src/mac.cc
    src/async_stop.cc
)

if (NOT WIN32)
    target_sources(qbox
        PRIVATE
        src/xgmac.cc
        src/dwmac.cc
        src/phy.cc
        src/backends/tap.cc
    )
endif()

if (WIN32)
    target_compile_options(qbox PRIVATE -Wall -DSC_WIN_DLL -DSC_INCLUDE_DYNAMIC_PROCESSES /vmg)
else ()
    target_compile_options(qbox PRIVATE
        -Wall
        "$<$<CONFIG:Debug>:-O0;-g3;-ggdb>"
    )
endif ()

target_include_directories(qbox
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/libqbox
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/include
)

target_link_libraries(qbox
    SystemC::SystemC
    gs
    qemu-cxx
    dl
)

if (slirp_FOUND)
    target_sources(qbox PRIVATE src/backends/slirp.cc)
    target_link_libraries(qbox slirp)
endif()

if (NOT WIN32)
    target_link_libraries(qbox pthread SystemC::CCI)
endif ()

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/libqbox" DESTINATION include)

if (WIN32)
    install(TARGETS qbox DESTINATION lib/$<IF:$<CONFIG:Debug>,debug,release>)
else ()
    install(TARGETS qbox DESTINATION lib)
endif ()
