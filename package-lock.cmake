# CPM Package Lock
# This file should be committed to version control

# libqbox
CPMDeclarePackage(libqbox
  NAME libqbox
# GIT_TAG v5.4.0
  GIT_TAG 5a364acc862deb8dea5943f0cc7d5e7cf1fe5ecd
  GIT_REPOSITORY ${GREENSOCS_GIT}qemu/libqbox.git
)
# libgsutils
CPMDeclarePackage(libgsutils
  NAME libgsutils
  GIT_TAG v2.3.0
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
# GIT_TAG v1.1.0
# GIT_TAG 8d78a9711597c96484e50288f6958a1c885483bd
  GIT_TAG 50314558169f9ef1acb80d20093783907393ca99
  GIT_REPOSITORY ${GREENSOCS_GIT}qemu/qemu.git
  GIT_SUBMODULES CMakeLists.txt
)
# base-components
CPMDeclarePackage(base-components
  NAME base-components
  GIT_TAG v3.2.0
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
  GIT_REPOSITORY ${GREENSOCS_GIT}quic/components/ipcc.git
#  GIT_SHALLOW on
)
# QTB
CPMDeclarePackage(qtb
  NAME qtb
  GIT_TAG master
  GIT_REPOSITORY ${GREENSOCS_GIT}quic/components/qtb.git
#  GIT_SHALLOW on
)
