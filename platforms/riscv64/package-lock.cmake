# CPM Package Lock
# This file should be committed to version control

# libqemu
CPMDeclarePackage(libqemu
  NAME libqemu
  GIT_TAG libqemu-v7.0.50-v1.3.2
  GIT_REPOSITORY ${GREENSOCS_GIT}qemu.git
  GIT_SUBMODULES CMakeLists.txt
)
