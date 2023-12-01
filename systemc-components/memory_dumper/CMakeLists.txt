gs_create_dymod(memory_dumper)

target_include_directories(
    memory_dumper PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/systemc-components/memory/include>
  )
