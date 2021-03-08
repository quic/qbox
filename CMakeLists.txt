cmake_minimum_required(VERSION 3.14)
project(libgssync  VERSION 1.0  LANGUAGES CXX)

##############################################
# ----- fetch GreenSocs cmake BOILERPLATE
##############################################
include(FetchContent)
FetchContent_Declare(gs-cmake
   GIT_REPOSITORY https://git.bar.greensocs.com/greensocs/cmake-boilerplate.git
   GIT_TAG master
)
FetchContent_MakeAvailable(gs-cmake)
include(${gs-cmake_SOURCE_DIR}/cmake/gs-boilerplate.cmake)
##############################################


gs_systemc()


# Other Dependencies

CPMAddPackage("https://git.bar.greensocs.com/greensocs/libgsutils.git#master")


add_library(${PROJECT_NAME} SHARED
   src/qkmultithread.cc
   src/qk_factory.cc
   src/pre_suspending_sc_support.cc
)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)

target_link_libraries(${PROJECT_NAME} PRIVATE libgsutils)

gs_target_options()
gs_enable_testing()
gs_export()
