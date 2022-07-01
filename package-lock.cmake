# CPM Package Lock
# This file should be committed to version control

# SystemCLanguage (unversioned)
# CPMDeclarePackage(SystemCLanguage
#  NAME SystemCLanguage
#  GIT_TAG async_suspendable
#  GIT_REPOSITORY git@git.greensocs.com:accellera/systemc.git
#  GIT_SHALLOW True
#  OPTIONS
#    "ENABLE_SUSPEND_ALL"
#    "ENABLE_PHASE_CALLBACKS"
#)

# RapidJSON (unversioned)
# CPMDeclarePackage(RapidJSON
#  NAME RapidJSON
#  GIT_TAG e0f68a435610e70ab5af44fc6a90523d69b210b3
#  GIT_REPOSITORY https://github.com/Tencent/rapidjson
#  GIT_SHALLOW FALSE
#  OPTIONS
#    "RAPIDJSON_BUILD_TESTS OFF"
#    "RAPIDJSON_BUILD_DOC OFF"
#    "RAPIDJSON_BUILD_EXAMPLES OFF"
#)

# SystemCCCI (unversioned)
# CPMDeclarePackage(SystemCCCI
#  NAME SystemCCCI
#  GIT_TAG accellera-automake-flow
#  GIT_REPOSITORY git@git.greensocs.com:accellera/cci.git
#  GIT_SHALLOW True
#  OPTIONS
#    "SYSTEMCCCI_BUILD_TESTS OFF"
#)

# libgsutils (unversioned)
CPMDeclarePackage(libgsutils
 NAME libgsutils
 GIT_TAG master
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
# CPMDeclarePackage(googletest
#  GIT_TAG main
#  GITHUB_REPOSITORY google/googletest
#  EXCLUDE_FROM_ALL YES
#)

# libgssync (unversioned)
CPMDeclarePackage(libgssync
 NAME libgssync
 GIT_TAG master
 GIT_REPOSITORY ${GREENSOCS_GIT}greensocs/libgssync.git
 GIT_SHALLOW on
)

# libqemu-cxx (unversioned)
CPMDeclarePackage(libqemu-cxx
 NAME libqemu-cxx
 GIT_TAG master
 GIT_REPOSITORY ${GREENSOCS_GIT}qemu/libqemu-cxx.git
 GIT_SHALLOW on
)

# libqemu
CPMDeclarePackage(libqemu
  NAME libqemu
  GIT_TAG libqemu/v7.0.50
  GIT_REPOSITORY ${GREENSOCS_GIT}qemu/qemu.git
  GIT_SUBMODULES "CMakeLists.txt"
  GIT_SHALLOW on
)

# base-components (unversioned)
CPMDeclarePackage(base-components
 NAME base-components
 GIT_TAG master
 GIT_REPOSITORY ${GREENSOCS_GIT}greensocs/components/base-components.git
 GIT_SHALLOW on
)

# keystone
CPMDeclarePackage(keystone
  NAME keystone
  GIT_TAG 0.9.2
  GIT_REPOSITORY https://github.com/keystone-engine/keystone.git
  GIT_SHALLOW TRUE
  OPTIONS
    "BUILD_LIBS_ONLY"
)
