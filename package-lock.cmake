# CPM Package Lock
# This file should be committed to version control

# This is a hack to workaround a CMake issue. We specify an existing file which
# is _not_ a submodule so that FetchContent does not init any submodule.
# See https://gitlab.kitware.com/cmake/cmake/-/issues/20579

# qemu (unversioned)
CPMDeclarePackage(qemu
    NAME libqemu
    GIT_TAG v3.0.0
    GIT_REPOSITORY git@git.greensocs.com:qemu/qemu.git
    GIT_SUBMODULES "CMakeLists.txt"
    GIT_SHALLOW on
)
