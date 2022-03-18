# CPM Package Lock
# This file should be committed to version control

# libqbox
CPMDeclarePackage(libqbox
  NAME libqbox
  GIT_TAG 052da099382394b782dc8f8782d74a67ea51204b
  GIT_REPOSITORY ${GREENSOCS_GIT}qemu/libqbox.git
)
# libgsutils
CPMDeclarePackage(libgsutils
  NAME libgsutils
  GIT_TAG v2.2.0
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
  GIT_TAG v2.1.0
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
  GIT_TAG libqemu/v6.2.0
  GIT_REPOSITORY ${GREENSOCS_GIT}customers/qualcomm/qualcomm-qemu/qemu-hexagon.git
  GIT_SUBMODULES CMakeLists.txt
)
# base-components
CPMDeclarePackage(base-components
  NAME base-components
  GIT_TAG v3.1.0
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
  GIT_TAG v3.1.1
  GIT_REPOSITORY ${GREENSOCS_GIT}qemu/extra-components.git
  GIT_SHALLOW on
)

# IPCC
CPMDeclarePackage(ipcc
  NAME ipcc
  GIT_TAG 491836963e3b4eada8005df098fcf4d15e18c591
  GIT_REPOSITORY ${GREENSOCS_GIT}customers/qualcomm/ipcc.git
#  GIT_SHALLOW on
)
# QTB
CPMDeclarePackage(qtb
  NAME qtb
  GIT_TAG 09a336d254d6bab8e3593d399a365d87e5ee6a38
  GIT_REPOSITORY ${GREENSOCS_GIT}customers/qualcomm/qtb.git
#  GIT_SHALLOW on
)
