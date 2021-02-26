

CPMAddPackage("https://git.bar.greensocs.com/greensocs/components/base-components.git#master")

add_executable(checker main.cc)
add_test(
   NAME integration_test
   COMMAND checker --gs_luafile ${CMAKE_CURRENT_SOURCE_DIR}/tests.lua
)
target_link_libraries(checker
   PRIVATE
      systemc
      cci
      libgsutils
      base-components
      libgssync
)
target_include_directories(checker
   PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include
)
