#find_package(SystemC REQUIRED COMPONENTS CCI)
#find_package(Lua REQUIRED)

#add_subdirectory(../../libssync libssync-build EXCLUDE_FROM_ALL)

add_executable(checker main.cc)
target_compile_definitions(checker PUBLIC HAS_LUA)
add_test(NAME integration_test
         COMMAND checker --gs_luafile ${CMAKE_CURRENT_SOURCE_DIR}/tests.lua)
target_link_libraries(checker
        PRIVATE
                systemc
                cci
                libgsutils
                pthread
                base-components
                lua
                gssync
#                ${LUA_LIBRARIES}
        )
target_include_directories(checker
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include
        )
include_directories(${LUA_INCLUDE_DIR})

#set_property(
#        TARGET checker
#        PROPERTY INTERFACE_LINK_LIBRARIES
#        "${CMAKE_BUILD_DIRECTORY}/../libssync.so"
#)
