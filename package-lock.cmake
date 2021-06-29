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
# libqbox
CPMDeclarePackage(libqbox
  GIT_TAG v2.0.0
  GIT_REPOSITORY git@git.greensocs.com:qemu/libqbox.git
  GIT_SHALLOW on
)
# libgsutils
CPMDeclarePackage(libgsutils
  GIT_TAG v2.0.0
  GIT_REPOSITORY git@git.greensocs.com:greensocs/libgsutils.git
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
#  GIT_TAG master
#  GITHUB_REPOSITORY google/googletest
#  EXCLUDE_FROM_ALL YES
#)
# libgssync
CPMDeclarePackage(libgssync
  GIT_TAG v2.0.0
  GIT_REPOSITORY git@git.greensocs.com:greensocs/libgssync.git
  GIT_SHALLOW on
)
# libqemu-cxx
CPMDeclarePackage(libqemu-cxx
  GIT_TAG v2.0.0
  GIT_REPOSITORY git@git.greensocs.com:qemu/libqemu-cxx.git
  GIT_SHALLOW on
)
# libqemu
CPMDeclarePackage(libqemu
  GIT_TAG v2.0.0
  GIT_REPOSITORY git@git.greensocs.com:qemu/libqemu.git
  GIT_SHALLOW on
  GIT_SUBMODULES CMakeLists.txt
)
# base-components
CPMDeclarePackage(base-components
  GIT_TAG v2.0.0
  GIT_REPOSITORY git@git.greensocs.com:greensocs/components/base-components.git
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
