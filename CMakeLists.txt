cmake_minimum_required(VERSION 3.19)
project(qqvp VERSION 1.0)

##############################################
# ----- fetch GreenSocs cmake BOILERPLATE
##############################################
set(PACKAGE_PATH "qqvp")
set(GREENSOCS_GIT "" CACHE STRING "Git directory from which to clone all gs repositoies")

execute_process(COMMAND git config --get remote.origin.url
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
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

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules")

gs_systemc()

gs_addpackage(qbox)

find_package(LibZip REQUIRED)

add_library(${PROJECT_NAME} INTERFACE)
add_executable(vp
    src/main.cc
)

execute_process(
    COMMAND sh -c "{ git describe --dirty --tags --always --abbrev || cat VERSION; } | tr -d '\n'"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE QQVP_VERSION
    COMMAND_ERROR_IS_FATAL ANY)
message(STATUS "QQVP_VERSION = ${QQVP_VERSION}")
target_compile_definitions(vp PRIVATE -DQQVP_VERSION="${QQVP_VERSION}")
target_include_directories(${PROJECT_NAME} INTERFACE ${LIBZIP_INCLUDE_DIRS})

target_include_directories(
    ${PROJECT_NAME} INTERFACE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(${PROJECT_NAME} INTERFACE
    ${TARGET_LIBS}
    ${LIBZIP_LIBRARY}
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

# Apply HVF entitlements on MacOS
if(APPLE)
    add_custom_command(
        TARGET vp
        POST_BUILD
        COMMAND codesign
                --entitlements ${libqemu_SOURCE_DIR}/accel/hvf/entitlements.plist
                --force -s - $<TARGET_FILE:vp>
        COMMENT "Applying HVF entitlements to $<TARGET_FILE:vp>"
    )
endif()

install (TARGETS vp DESTINATION ${CMAKE_INSTALL_BINDIR})
file(GLOB REF_USER_CONF_FILES "${CMAKE_SOURCE_DIR}/configs/*.lua")
install(FILES ${REF_USER_CONF_FILES} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/examples)

if(APPLE)
	# Workardound for <install_dir>/bin/vp getting 'Killed 9' on MacOS.
	# This seems to happen because, on MacOS, cp will reuse the destination
	# inode, making the codesign invalid on the installed vp. More info at:
	# https://developer.apple.com/forums/thread/705117#711951022
	#
	# We work around this adding an extra install command to overwrite the
	# installed vp with the 'install' command, which does what we want.
	install(CODE "execute_process(COMMAND install \"${CMAKE_BINARY_DIR}/vp\" \"${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/vp\")")
endif()

gs_enable_testing()
