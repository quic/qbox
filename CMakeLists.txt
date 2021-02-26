cmake_minimum_required(VERSION 3.14)
project(libgsutils)



add_library(libgsutils INTERFACE)
target_include_directories(libgsutils INTERFACE include)

CPMAddPackage("gh:lua/lua#v5.3.5")
if (lua_ADDED)
  # lua has no CMake support, so we create our own target

  FILE(GLOB lua_sources ${lua_SOURCE_DIR}/*.c)
  list(REMOVE_ITEM lua_sources "${lua_SOURCE_DIR}/lua.c" "${lua_SOURCE_DIR}/luac.c")
  add_library(lua STATIC ${lua_sources})

  target_include_directories(lua PUBLIC $<BUILD_INTERFACE:${lua_SOURCE_DIR}>)

  target_compile_definitions(libgsutils INTERFACE HAS_LUA)
  target_link_libraries(libgsutils INTERFACE lua)
endif()

# work arround as googletest supports old cmake
if (APPLE)
       set(CMAKE_MACOSX_RPATH ON)
endif (APPLE)
# using master branch as it has fixes for cmake
CPMAddPackage("gh:google/googletest#master")
if (googletest_ADDED)
  target_link_libraries(libgsutils INTERFACE gtest gmock)
endif()  
