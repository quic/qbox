cmake_minimum_required(VERSION 3.14)
project(libgsutils)

CPMAddPackage(
  NAME lua
  GIT_REPOSITORY https://github.com/lua/lua.git
  VERSION 5.3.5
  DOWNLOAD_ONLY YES
)

add_library(libgsutils INTERFACE)
target_include_directories(libgsutils INTERFACE include)

if (lua_ADDED)
  # lua has no CMake support, so we create our own target

  FILE(GLOB lua_sources ${lua_SOURCE_DIR}/*.c)
  list(REMOVE_ITEM lua_sources "${lua_SOURCE_DIR}/lua.c" "${lua_SOURCE_DIR}/luac.c")
  add_library(lua STATIC ${lua_sources})

  target_include_directories(lua PUBLIC $<BUILD_INTERFACE:${lua_SOURCE_DIR}>)

  target_compile_definitions(libgsutils INTERFACE HAS_LUA)
  target_link_libraries(libgsutils INTERFACE lua)
endif()


