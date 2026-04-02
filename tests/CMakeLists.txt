# Place test executables under <build>/tests/... mirroring the source tree,
# rather than in the build root.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "")

add_subdirectory(components)
add_subdirectory(sync)
add_subdirectory(utils)
if(NOT WITHOUT_QEMU)
    add_subdirectory(qbox)
endif()