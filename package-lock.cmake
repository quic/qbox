# CPM Package Lock
# This file should be committed to version control

# qbox
CPMDeclarePackage(qbox
  NAME qbox
  GIT_TAG main
  GIT_REPOSITORY ${GREENSOCS_GIT}qbox.git
)

# libqemu
CPMDeclarePackage(libqemu
  NAME libqemu
  GIT_TAG libqemu-v7.2-v0.5.4
  GIT_REPOSITORY ${GREENSOCS_GIT}qemu/qemu.git
  GIT_SUBMODULES CMakeLists.txt
)
