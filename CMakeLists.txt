cmake_minimum_required(VERSION 3.14)
project(qqvp VERSION 1.0)

##############################################
# ----- fetch GreenSocs cmake BOILERPLATE
##############################################
set(PACKAGE_PATH "qqvp")
set(GREENSOCS_GIT "" CACHE STRING "Git directory from which to clone all gs repositoies")

execute_process(COMMAND git config --get remote.origin.url
    OUTPUT_STRIP_TRAILING_WHITESPACE
    OUTPUT_VARIABLE GIT_URL)

string(REGEX REPLACE "(.*)/${PACKAGE_PATH}.git$" "\\1/" GREENSOCS_GIT_ORIGIN "${GIT_URL}")
string(REGEX REPLACE "(.*)/sandboxes/[^\/]*/(.*)" "\\1/" GREENSOCS_GIT_ORIGIN "${GREENSOCS_GIT_ORIGIN}")

message(STATUS "GREENSOCS_GIT_ORIGIN = ${GREENSOCS_GIT_ORIGIN}")

if ( "${GREENSOCS_GIT}" STREQUAL "")
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

set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

set(LIBQEMU_TARGETS aarch64 hexagon)

gs_systemc()

gs_addpackage(qbox)

add_library(${PROJECT_NAME} INTERFACE)
add_executable(vp
    src/main.cc
)

target_include_directories(
    ${PROJECT_NAME} INTERFACE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(${PROJECT_NAME} INTERFACE
    ${TARGET_LIBS}
)

gs_export()

if(APPLE)
    target_link_options(vp PRIVATE "LINKER:-dead_strip_dylibs")
else()
    target_link_options(vp PRIVATE "LINKER:-as-needed")
endif()

target_link_libraries(vp
    ${TARGET_LIBS}
)

install (TARGETS vp DESTINATION ${CMAKE_INSTALL_BINDIR})
file(GLOB REF_USER_CONF_FILES "${CMAKE_SOURCE_DIR}/configs/*.lua")
install(FILES ${REF_USER_CONF_FILES} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/examples)

gs_enable_testing()
