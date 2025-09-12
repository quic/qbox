# CPM Package Lock
# This file should be committed to version control

# SystemC
CPMDeclarePackage(SystemCLanguage
  NAME SystemCLanguage
  GIT_TAG febde7a3e007ce3030cf574a4cb6fcc13d0ed820 # TODO: Replace with release > 3.0.2
  GIT_REPOSITORY https://github.com/accellera-official/systemc.git
  GIT_SHALLOW OFF
  OPTIONS
      "ENABLE_SUSPEND_ALL"
      "ENABLE_PHASE_CALLBACKS"
)

# SCP
CPMDeclarePackage(SCP
  NAME SCP
  GIT_REPOSITORY https://github.com/accellera-official/systemc-common-practices.git
  GIT_TAG 6830c915bb691d9b505b17ac631f1ff305fe9c17
  GIT_SHALLOW OFF
)

# qemu (unversioned)
CPMDeclarePackage(qemu
    NAME libqemu
    GIT_REPOSITORY ${GREENSOCS_GIT}${QEMU_PATH_NAME}.git
    GIT_TAG libqemu-v10.1-v0.11
    GIT_SUBMODULES CMakeLists.txt
    GIT_SHALLOW ON
)
