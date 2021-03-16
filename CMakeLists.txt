cmake_minimum_required(VERSION 3.14)
project(gsutils VERSION 1.0 LANGUAGES CXX C)


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

add_library(${PROJECT_NAME} INTERFACE)

gs_systemc()

# Other Dependencies

CPMAddPackage("gh:lua/lua#v5.4.2")
if (lua_ADDED)
  # lua has no CMake support, so we create our own target
  FILE(GLOB lua_sources ${lua_SOURCE_DIR}/*.c)
  list(REMOVE_ITEM lua_sources "${lua_SOURCE_DIR}/lua.c" "${lua_SOURCE_DIR}/luac.c" "${lua_SOURCE_DIR}/luaone.c")
  add_library(lua STATIC ${lua_sources})
  target_include_directories(lua PUBLIC $<BUILD_INTERFACE:${lua_SOURCE_DIR}>)
  target_compile_definitions(${PROJECT_NAME} INTERFACE HAS_LUA)
  target_link_libraries(${PROJECT_NAME} INTERFACE lua)
  install(TARGETS lua EXPORT ${PROJECT_NAME}Targets)
endif()

# work arround as googletest supports old cmake
if (APPLE)
       set(CMAKE_MACOSX_RPATH ON)
endif (APPLE)
# using master branch as it has fixes for cmake
CPMAddPackage("gh:google/googletest#master")
if (googletest_ADDED)
  target_link_libraries(${PROJECT_NAME} INTERFACE gtest gmock)
endif()

target_include_directories(
   ${PROJECT_NAME} INTERFACE
   $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
   $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

gs_enable_testing()
gs_export()