cmake_minimum_required(VERSION 3.14)
project(libgssync  VERSION 1.0  LANGUAGES CXX)

##############################################
# ----- BOILERPLATE
# should be included on all projects
# sets up test, and sub-project (CPM) support
# Sets C++ standard to 14.
##############################################
if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
   message( FATAL_ERROR "Please use a build directory." )
endif()

include(FetchContent)
include(CTest)

FetchContent_Declare(cpm-cmake
   GIT_REPOSITORY https://github.com/cpm-cmake/CPM.cmake.git
   GIT_TAG v0.31.1
)
FetchContent_MakeAvailable(cpm-cmake)
include(${cpm-cmake_SOURCE_DIR}/cmake/CPM.cmake)

CPMAddPackage("gh:TheLartians/PackageProject.cmake@1.4.1")

set (CMAKE_CXX_STANDARD 14 CACHE STRING
   "C++ standard to build all targets.")
set (CMAKE_CXX_STANDARD_REQUIRED ON CACHE BOOL
   "The with CMAKE_CXX_STANDARD selected C++ standard is a requirement.")
mark_as_advanced (CMAKE_CXX_STANDARD_REQUIRED)

include(GNUInstallDirs)

##############################################
# ----- END BOILERPLATE
##############################################

# dependencies

CPMFindPackage(
   NAME systemc
   GIT_REPOSITORY https://github.com/greensocs/systemc.git
   GIT_TAG async_suspendable
   GIT_SHALLOW True
   OPTIONS "ENABLE_SUSPEND_ALL"
   OPTIONS "ENABLE_PHASE_CALLBACKS"
)
CPMFindPackage(
   NAME cci
   GIT_REPOSITORY git@git.bar.greensocs.com:mark/cci.git
   GIT_TAG accellera-automake-flow
   GIT_SHALLOW True
)

CPMAddPackage("https://git.bar.greensocs.com/greensocs/libgsutils.git#master")


add_library(${PROJECT_NAME} SHARED
   src/qkmultithread.cc
   src/qk_factory.cc
   src/pre_suspending_sc_support.cc
)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)

target_compile_definitions(${PROJECT_NAME} PUBLIC SC_INCLUDE_DYNAMIC_PROCESSES)
target_link_libraries(${PROJECT_NAME} PRIVATE systemc cci libgsutils base-components)

target_include_directories(
   ${PROJECT_NAME} PUBLIC 
   $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
   $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# hack for MSVC
target_compile_options(
   ${PROJECT_NAME}
   PUBLIC
   $<$<CXX_COMPILER_ID:MSVC>:/vmg /MP>
)

# ----- EXPORT PROJECT
string(TOLOWER ${PROJECT_NAME}/version.h VERSION_HEADER_LOCATION)
packageProject(
   NAME ${PROJECT_NAME}
   VERSION ${PROJECT_VERSION}
   NAMESPACE ${PROJECT_NAME}
   BINARY_DIR ${PROJECT_BINARY_DIR}
   INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include
   INCLUDE_DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
   VERSION_HEADER "${VERSION_HEADER_LOCATION}"
   COMPATIBILITY SameMajorVersion
   DEPENDENCIES "systemc 2.3.1"
)


if(BUILD_TESTING AND ("${PROJECT_NAME}" STREQUAL "${CMAKE_PROJECT_NAME}"))
   enable_testing()
   add_subdirectory(tests)
endif()

