cmake_minimum_required(VERSION 3.14)
project(greensocs-base-vp)

set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

set(LIBQEMU_TARGETS aarch64 hexagon)

add_subdirectory(../ ${CMAKE_BINARY_DIR}/_deps/quic-build)

add_executable(vp
    src/main.cc
)
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
