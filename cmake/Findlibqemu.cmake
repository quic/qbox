#.rst:
# Findlibqemu
# -----------
#
# Try to find libqemu

set(_LIBQEMU_PATHS PATHS
    ${LIBQEMU_PREFIX}
    $ENV{LIBQEMU_PREFIX}
)

find_path(LIBQEMU_INCLUDE_DIR libqemu/libqemu.h ${_LIBQEMU_PATHS} PATH_SUFFIXES include)

if (LIBQEMU_INCLUDE_DIR AND EXISTS "${LIBQEMU_INCLUDE_DIR}/libqemu/libqemu.h")
    set(LIBQEMU_VERSION "1.0.0")
endif ()

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LIBQEMU
    REQUIRED_VARS LIBQEMU_INCLUDE_DIR
    VERSION_VAR LIBQEMU_VERSION)

add_library(libqemu INTERFACE IMPORTED)
set_target_properties(libqemu PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${LIBQEMU_INCLUDE_DIR};${LIBQEMU_INCLUDE_DIR}/libqemu"
)

