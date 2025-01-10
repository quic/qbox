# CPM Package Lock
# This file should be committed to version control

# This is a hack to workaround a CMake issue. We specify an existing file which
# is _not_ a submodule so that FetchContent does not init any submodule.
# See https://gitlab.kitware.com/cmake/cmake/-/issues/20579

# cci
CPMDeclarePackage(SystemCCCI
  NAME SystemCCCI
  GIT_TAG v1.0.1
  GIT_REPOSITORY https://github.com/accellera-official/cci.git
  GIT_SHALLOW on
  OPTIONS
      "SYSTEMCCCI_BUILD_TESTS OFF"
)

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
    GIT_TAG libqemu-v9.1-v0.5
    GIT_SUBMODULES CMakeLists.txt
    GIT_SHALLOW on
)
