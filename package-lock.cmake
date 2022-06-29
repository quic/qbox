# CPM Package Lock
# This file should be committed to version control

# libqbox
CPMDeclarePackage(libqbox
  NAME libqbox
  GIT_TAG v5.4.1
  GIT_REPOSITORY ${GREENSOCS_GIT}qemu/libqbox.git
)
# libgsutils
CPMDeclarePackage(libgsutils
  NAME libgsutils
  GIT_TAG v2.3.1
  GIT_REPOSITORY ${GREENSOCS_GIT}greensocs/libgsutils.git
  GIT_SHALLOW on
)
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
# libgssync
CPMDeclarePackage(libgssync
  NAME libgssync
  GIT_TAG v2.1.2
  GIT_REPOSITORY ${GREENSOCS_GIT}greensocs/libgssync.git
  GIT_SHALLOW on
)
# libqemu-cxx
CPMDeclarePackage(libqemu-cxx
  NAME libqemu-cxx
  GIT_TAG v3.6.0
  GIT_REPOSITORY ${GREENSOCS_GIT}qemu/libqemu-cxx.git
)
# libqemu
CPMDeclarePackage(libqemu
  NAME libqemu
  GIT_REPOSITORY ${GREENSOCS_GIT}qemu/qemu.git
  GIT_TAG v2.1.0
  GIT_SUBMODULES CMakeLists.txt
)
# base-components
CPMDeclarePackage(base-components
  NAME base-components
  GIT_TAG v3.3.2
  GIT_REPOSITORY ${GREENSOCS_GIT}greensocs/components/base-components.git
#  GIT_SHALLOW on
)

CPMDeclarePackage(systemc-uarts
  NAME systemc-uarts
  GIT_TAG v2.1.2
  GIT_REPOSITORY ${GREENSOCS_GIT}greensocs/components/systemc-uarts.git
  GIT_SHALLOW on
)

# extra-components
CPMDeclarePackage(extra-components
  NAME extra-components
  GIT_TAG v3.2.0
  GIT_REPOSITORY ${GREENSOCS_GIT}qemu/extra-components.git
  GIT_SHALLOW on
)

# IPCC
CPMDeclarePackage(ipcc
  NAME ipcc
  GIT_REPOSITORY ${GREENSOCS_GIT}quic/components/ipcc.git
  GIT_TAG v1.0.0
#  GIT_SHALLOW on
)
# QTB
CPMDeclarePackage(qtb
  NAME qtb
  GIT_REPOSITORY ${GREENSOCS_GIT}quic/components/qtb.git
  GIT_TAG v1.0.0
#  GIT_SHALLOW on
)
