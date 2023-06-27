cmake_minimum_required(VERSION 3.14)
project(genmods VERSION 1.0)

##############################################
# ----- fetch GreenSocs cmake BOILERPLATE
##############################################
set(PACKAGE_PATH "sandboxes/ipcat_model_maker")
set(GREENSOCS_GIT "" CACHE STRING "Git directory from which to clone all gs repositoies")

execute_process(COMMAND git config --get remote.origin.url
    OUTPUT_STRIP_TRAILING_WHITESPACE
    OUTPUT_VARIABLE GIT_URL)
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

# set (IPCATFILE "${PROJECT_SOURCE_DIR}/Makena.txt" CACHE STRING "Source IPCAT file using %s 0x%08x 0x%08x %s 0x%08x 0x%08x 0x%08x  %s  module.name, module.address, module.size, name, offset, reset_value, mask, access")

gs_systemc()
gs_addpackage(greensocs/libgsutils)
gs_addpackage(greensocs/components/base-components)

find_package(libzip REQUIRED)

#file(GLOB tplt_SRCS
#    "${reg_model_maker_SOURCE_DIR}/generator.pl"
#)

#add_custom_command(OUTPUT "${CMAKE_BINARY_DIR}/gen/done"
#    COMMAND "${reg_model_maker_SOURCE_DIR}/generator.pl" "${IPCATFILE}" "reg_model_maker"
#    DEPENDS ${IPCATFILE} ${tplt_SRCS}
#)
#add_custom_target(generate_modules ALL DEPENDS  "${CMAKE_BINARY_DIR}/gen/done")

#execute_process(
#    WORKING_DIRECTORY ${CMAKE_BINARY_DIR} 
#    COMMAND "${reg_model_maker_SOURCE_DIR}/generator.pl" "${IPCATFILE}" "reg_model_maker"
#)

add_library(${PROJECT_NAME} INTERFACE)

target_include_directories(
    ${PROJECT_NAME} INTERFACE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(${PROJECT_NAME} INTERFACE ${TARGET_LIBS} libzip::zip)

gs_export()
gs_enable_testing()
