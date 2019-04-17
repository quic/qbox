#.rst:
# Findlibqemu
# -----------
#
# Try to find libqemu

set(_LIBQEMU_PATHS PATHS
    ${LIBQEMU_PREFIX}
    $ENV{LIBQEMU_PREFIX}
)

find_path(LIBQEMU_INCLUDE_DIR libqemu.h ${_LIBQEMU_PATHS} PATH_SUFFIXES include/libqemu)

if (NOT LIBQEMU_LIBRARIES)
    find_library(LIBQEMU_LIBRARY_RELEASE NAMES qemu-system-arm ${_LIBQEMU_PATHS} PATH_SUFFIXES lib/rabbits)
    include(SelectLibraryConfigurations)
    SELECT_LIBRARY_CONFIGURATIONS(LIBQEMU)
endif ()

if (LIBQEMU_INCLUDE_DIR AND EXISTS "${LIBQEMU_INCLUDE_DIR}/libqemu/libqemu.h")
    set(LIBQEMU_VERSION "1.0.0")
endif ()

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LIBQEMU
    REQUIRED_VARS LIBQEMU_LIBRARIES LIBQEMU_INCLUDE_DIR
    VERSION_VAR LIBQEMU_VERSION)

