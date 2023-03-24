# CPM Package Lock
# This file should be committed to version control

# cci
CPMDeclarePackage(cci
  NAME cci
  GIT_TAG v1.0.0.1
  GIT_REPOSITORY ${ACCELLERA_GIT}accellera/cci.git
  GIT_SHALLOW on
)

# SystemC
CPMDeclarePackage(SystemCLanguage
  NAME SystemCLanguage
  GIT_TAG v2.3.4
  GIT_REPOSITORY ${ACCELLERA_GIT}accellera/systemc.git
  GIT_SHALLOW on
  OPTIONS
      "ENABLE_SUSPEND_ALL"
      "ENABLE_PHASE_CALLBACKS"
)

# SCP
CPMDeclarePackage(SCP
  NAME SCP
  GIT_TAG v0.1.0
  GIT_REPOSITORY ${ACCELLERA_GIT}accellera/systemc-common-practices.git
  GIT_SHALLOW on
)

# qbox
CPMDeclarePackage(qbox
  NAME qbox
  GIT_TAG main
  GIT_REPOSITORY ${GREENSOCS_GIT}qbox.git
  GIT_SHALLOW on
)

# libqemu
CPMDeclarePackage(libqemu
  NAME libqemu
  GIT_TAG libqemu-v7.2-v0.5.4
  GIT_REPOSITORY ${GREENSOCS_GIT}qemu/qemu.git
  GIT_SUBMODULES CMakeLists.txt
  GIT_SHALLOW on
)
