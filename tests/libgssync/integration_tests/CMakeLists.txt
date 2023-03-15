add_executable(checker main.cc)
add_test(
   NAME integration_test
   COMMAND checker --gs_luafile ${CMAKE_CURRENT_SOURCE_DIR}/tests.lua
)
target_link_libraries(checker
   PRIVATE
      ${TARGET_LIBS}
)
target_include_directories(checker
   PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/greensocs/
)