cmake_minimum_required(VERSION 3.14)
project(gssync  VERSION 1.0  LANGUAGES CXX)

##############################################
# ----- fetch GreenSocs cmake BOILERPLATE
##############################################
if (NOT gs-cmake_SOURCE_DIR)
   include(FetchContent)
   FetchContent_Declare(gs-cmake
      GIT_REPOSITORY git@git.greensocs.com:greensocs/cmake-boilerplate.git
      GIT_TAG master
   )
   FetchContent_MakeAvailable(gs-cmake)
endif()
include(${gs-cmake_SOURCE_DIR}/cmake/gs-boilerplate.cmake)
##############################################


gs_systemc()


# Other Dependencies

gs_addpackage(greensocs/libgsutils)

add_library(${PROJECT_NAME} SHARED
   src/qkmultithread.cc
   src/qk_factory.cc
   src/pre_suspending_sc_support.cc
)

target_include_directories(
   ${PROJECT_NAME} PUBLIC
   $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
   $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)

target_link_libraries(${PROJECT_NAME} PRIVATE GreenSocs::gsutils)
target_link_libraries(${PROJECT_NAME} PRIVATE SystemC::systemc SystemC::cci)

gs_enable_testing()
gs_export()
