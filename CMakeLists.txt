cmake_minimum_required(VERSION 3.14)
project(gssync  VERSION 1.0  LANGUAGES CXX)


# ----- BOILERPLATE
if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  message(
    FATAL_ERROR
      "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there."
  )
endif()

include(FetchContent)
include(CTest)

FetchContent_Declare(cpm-cmake
GIT_REPOSITORY https://github.com/cpm-cmake/CPM.cmake.git
GIT_TAG v0.31.1)
FetchContent_MakeAvailable(cpm-cmake)
include(${cpm-cmake_SOURCE_DIR}/cmake/CPM.cmake)

CPMAddPackage("gh:TheLartians/PackageProject.cmake@1.4.1")

# ----- END BOILERPLATE

set (CMAKE_CXX_STANDARD 14 CACHE STRING
     "C++ standard to build all targets.")
set (CMAKE_CXX_STANDARD_REQUIRED ON CACHE BOOL
     "The with CMAKE_CXX_STANDARD selected C++ standard is a requirement.")
mark_as_advanced (CMAKE_CXX_STANDARD_REQUIRED)

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
CPMAddPackage("https://git.bar.greensocs.com/greensocs/components/base-components.git#master")


add_library(gssync SHARED
    src/qkmultithread.cc
    src/qk_factory.cc
    src/pre_suspending_sc_support.cc
)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(gssync PRIVATE Threads::Threads)

target_compile_definitions(gssync PUBLIC SC_INCLUDE_DYNAMIC_PROCESSES)
target_link_libraries(gssync PRIVATE systemc cci libgsutils base-components)

target_include_directories(
    gssync PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                 $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}>
)

# ----- EXPORT PROJECT
string(TOLOWER ${PROJECT_NAME}/version.h VERSION_HEADER_LOCATION)
packageProject(
  NAME ${PROJECT_NAME}
  VERSION ${PROJECT_VERSION}
  NAMESPACE ${PROJECT_NAME}
  BINARY_DIR ${PROJECT_BINARY_DIR}
  INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include
  INCLUDE_DESTINATION include/${PROJECT_NAME}-${PROJECT_VERSION}
  VERSION_HEADER "${VERSION_HEADER_LOCATION}"
  COMPATIBILITY SameMajorVersion
  DEPENDENCIES "systemc 2.3.1"
)


if(BUILD_TESTING AND ("${PROJECT_NAME}" STREQUAL "${CMAKE_PROJECT_NAME}"))
    enable_testing()
    add_subdirectory(tests)
endif()



#if (NOT WIN32)
#    target_link_libraries(gssync pthread)
#endif()
#
#if(WIN32)
#    target_compile_options(gssync PRIVATE -Wall -DSC_WIN_DLL -DSC_INCLUDE_DYNAMIC_PROCESSES /vmg)
#else()
#    target_compile_definitions(gssync PUBLIC SC_INCLUDE_DYNAMIC_PROCESSES)
#endif()
#
#install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/libgs" DESTINATION include)
#
#if (WIN32)
#    install(TARGETS gssync DESTINATION lib/$<IF:$<CONFIG:Debug>,debug,release>)
#else ()
#    install(TARGETS gssync DESTINATION lib)
#endif ()










