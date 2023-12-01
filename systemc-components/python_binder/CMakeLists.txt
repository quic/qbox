gs_create_dymod(python_binder)

target_link_libraries(python_binder PUBLIC
    ${PYBIND11_EMBED}
)

target_include_directories(
    python_binder PUBLIC
    $<BUILD_INTERFACE:${pybind11_SOURCE_DIR}/include>
)
