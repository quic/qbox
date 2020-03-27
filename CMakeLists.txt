cmake_minimum_required(VERSION 3.3)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

project(libqbox)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(SystemC REQUIRED)

add_library(qbox SHARED
    src/libqbox.cc
)

if (WIN32)
    target_compile_options(qbox PRIVATE -Wall -DSC_WIN_DLL -DSC_INCLUDE_DYNAMIC_PROCESSES /vmg)
else ()
    target_compile_options(qbox PRIVATE -Wall)
endif ()

target_include_directories(qbox
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/libqbox
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/include
)

target_link_libraries(qbox
    SystemC::SystemC
)

if (NOT WIN32)
    target_link_libraries(qbox pthread)
endif ()

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/libqbox" DESTINATION include)

if (WIN32)
    install(TARGETS qbox DESTINATION lib/$<IF:$<CONFIG:Debug>,debug,release>)
else ()
    install(TARGETS qbox DESTINATION lib)
endif ()
