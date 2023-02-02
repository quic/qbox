# CPM Package Lock
# This file should be committed to version control

# libqbox
CPMDeclarePackage(libqbox
  NAME libqbox
  GIT_TAG v7.2.0
  GIT_REPOSITORY ${GREENSOCS_GIT}qemu/libqbox.git
)
# libgsutils
CPMDeclarePackage(libgsutils
  NAME libgsutils
  GIT_TAG v2.6.2
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
  GIT_TAG v2.5.0
  GIT_REPOSITORY ${GREENSOCS_GIT}greensocs/libgssync.git
  GIT_SHALLOW on
)
# libqemu-cxx
CPMDeclarePackage(libqemu-cxx
  NAME libqemu-cxx
  GIT_TAG v4.2.2
  GIT_REPOSITORY ${GREENSOCS_GIT}qemu/libqemu-cxx.git
)
# libqemu
CPMDeclarePackage(libqemu
  NAME libqemu
  GIT_TAG libqemu-v7.0.50-v1.3.1
  GIT_REPOSITORY ${GREENSOCS_GIT}qemu/qemu.git
  GIT_SUBMODULES CMakeLists.txt
)
# base-components
CPMDeclarePackage(base-components
  NAME base-components
  GIT_TAG v4.2.1
  GIT_REPOSITORY ${GREENSOCS_GIT}greensocs/components/base-components.git
#  GIT_SHALLOW on
)

CPMDeclarePackage(systemc-uarts
  NAME systemc-uarts
  GIT_TAG v2.4.3
  GIT_REPOSITORY ${GREENSOCS_GIT}greensocs/components/systemc-uarts.git
  GIT_SHALLOW on
)

# extra-components
CPMDeclarePackage(extra-components
  NAME extra-components
  GIT_TAG v4.2.0
  GIT_REPOSITORY ${GREENSOCS_GIT}qemu/extra-components.git
  GIT_SHALLOW on
)

# IPCC
CPMDeclarePackage(ipcc
  NAME ipcc
  GIT_REPOSITORY ${GREENSOCS_GIT}quic/components/ipcc.git
  GIT_TAG v1.2.0
  GIT_SHALLOW on
)
# QTB
CPMDeclarePackage(qtb
  NAME qtb
  GIT_REPOSITORY ${GREENSOCS_GIT}quic/components/qtb.git
  GIT_TAG v1.2.0
  GIT_SHALLOW on
)
# CSR
CPMDeclarePackage(csr
  NAME csr
  GIT_REPOSITORY ${GREENSOCS_GIT}quic/components/csr.git
  GIT_TAG v1.2.0
  GIT_SHALLOW on
)
# smmu
CPMDeclarePackage(smmu500
  NAME smmu500
  GIT_REPOSITORY ${GREENSOCS_GIT}quic/components/smmu500.git
  GIT_TAG v0.2.0
  GIT_SHALLOW on
)

# qup
CPMDeclarePackage(qup
  NAME qup
  GIT_REPOSITORY ${GREENSOCS_GIT}quic/components/qup.git
  GIT_TAG v0.1.2
  GIT_SHALLOW on
)
# SCP
CPMDeclarePackage(SCP
  NAME SCP
  GIT_REPOSITORY ${GREENSOCS_GIT}accellera/systemc-common-practices.git
  GIT_TAG v0.2.0
  GIT_SHALLOW on
)
