# CPM Package Lock
# This file should be committed to version control

# libqbox
CPMDeclarePackage(libqbox
  NAME libqbox
  GIT_TAG 20352c91ae30619f2550834a5e5ec0bf55f62e03
  GIT_REPOSITORY ${GREENSOCS_GIT}qemu/libqbox.git
)
# libgsutils
CPMDeclarePackage(libgsutils
  NAME libgsutils
  GIT_TAG 7684941e6fe6aefc3f6693117cc6502e14b384a7
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
  GIT_TAG v2.1.1
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
  GIT_TAG d61c2c00caf7a9406a31512f0282587306346609
  GIT_REPOSITORY ${GREENSOCS_GIT}customers/qualcomm/qualcomm-qemu/qemu-hexagon.git
  GIT_SUBMODULES CMakeLists.txt
)
# base-components
CPMDeclarePackage(base-components
  NAME base-components
  GIT_TAG 1d308962aeb5db9c8e72a75d9e993b2bdacf1aeb
  GIT_REPOSITORY ${GREENSOCS_GIT}greensocs/components/base-components.git
#  GIT_SHALLOW on
)

CPMDeclarePackage(systemc-uarts
  NAME systemc-uarts
  GIT_TAG v2.1.1
  GIT_REPOSITORY ${GREENSOCS_GIT}greensocs/components/systemc-uarts.git
  GIT_SHALLOW on
)

# extra-components
CPMDeclarePackage(extra-components
  NAME extra-components
  GIT_TAG v3.1.3
  GIT_REPOSITORY ${GREENSOCS_GIT}qemu/extra-components.git
  GIT_SHALLOW on
)

# IPCC
CPMDeclarePackage(ipcc
  NAME ipcc
  GIT_TAG master
  GIT_REPOSITORY ${GREENSOCS_GIT}customers/qualcomm/ipcc.git
#  GIT_SHALLOW on
)
# QTB
CPMDeclarePackage(qtb
  NAME qtb
  GIT_TAG master
  GIT_REPOSITORY ${GREENSOCS_GIT}customers/qualcomm/qtb.git
#  GIT_SHALLOW on
)
