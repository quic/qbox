# CPM Package Lock
# This file should be committed to version control

# SystemC
CPMDeclarePackage(SystemCLanguage
  NAME SystemCLanguage
  GIT_TAG 3.0.0
  GIT_REPOSITORY https://github.com/accellera-official/systemc.git
  GIT_SHALLOW on
  OPTIONS
      "ENABLE_SUSPEND_ALL"
      "ENABLE_PHASE_CALLBACKS"
)

# SCP
CPMDeclarePackage(SCP
  NAME SCP
  GIT_TAG v0.1.0
  GIT_REPOSITORY https://github.com/accellera-official/systemc-common-practices.git
  GIT_SHALLOW on
)

# qemu (unversioned)
CPMDeclarePackage(qemu
    NAME libqemu
    GIT_REPOSITORY ${GREENSOCS_GIT}${QEMU_PATH_NAME}.git
    GIT_TAG libqemu-v9.1-v0.10
    GIT_SUBMODULES CMakeLists.txt
    GIT_SHALLOW on
)
