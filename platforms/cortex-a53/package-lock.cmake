# CPM Package Lock
# This file should be committed to version control

# lua
CPMDeclarePackage(lua
  GIT_TAG v5.4.2
  GITHUB_REPOSITORY lua/lua
  EXCLUDE_FROM_ALL YES
)
# googletest (unversioned)
CPMDeclarePackage(googletest
 GIT_TAG main
 GITHUB_REPOSITORY google/googletest
 EXCLUDE_FROM_ALL YES
)

# libqemu
CPMDeclarePackage(libqemu
  NAME libqemu
  GIT_TAG libqemu-v7.0.50-v1.3.2
  GIT_REPOSITORY ${GREENSOCS_GIT}qemu.git
  GIT_SUBMODULES CMakeLists.txt
)