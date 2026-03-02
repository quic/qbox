gs_create_dymod(generic_lua_model)
target_include_directories(
    generic_lua_model PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/systemc-components/gs_memory/include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/systemc-components/reg_router/include>
  )