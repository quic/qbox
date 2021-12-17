# CPM Package Lock
# This file should be committed to version control

# libqbox
CPMDeclarePackage(libqbox
  NAME libqbox
  GIT_TAG 77bb57ef4bb03d98bd07e60412336c2b276f2bf5
  GIT_REPOSITORY git@git.greensocs.com:qemu/libqbox.git
)
# libgsutils
CPMDeclarePackage(libgsutils
  NAME libgsutils
  GIT_TAG v2.1.2
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
CPMDeclarePackage(googletest
 GIT_TAG main
 GITHUB_REPOSITORY google/googletest
 EXCLUDE_FROM_ALL YES
)
# libgssync
CPMDeclarePackage(libgssync
  NAME libgssync
  GIT_TAG v2.1.0
  GIT_REPOSITORY git@git.greensocs.com:greensocs/libgssync.git
  GIT_SHALLOW on
)
# libqemu-cxx
CPMDeclarePackage(libqemu-cxx
  NAME libqemu-cxx
  GIT_TAG 285726489db7411ff57051df2032c014b328a968
  GIT_REPOSITORY git@git.greensocs.com:qemu/libqemu-cxx.git
)
# libqemu
CPMDeclarePackage(libqemu
  GIT_TAG 6452339b49f4605977c45818f7448d20d21be01f
  GIT_REPOSITORY git@git.greensocs.com:customers/qualcomm/qualcomm-qemu/qemu-hexagon.git
  GIT_SUBMODULES CMakeLists.txt
)
# base-components
CPMDeclarePackage(base-components
  NAME base-components
  GIT_TAG v2.1.2
  GIT_REPOSITORY git@git.greensocs.com:greensocs/components/base-components.git
  GIT_SHALLOW on
)

# extra-components
CPMDeclarePackage(extra-components
  NAME extra-components
  GIT_TAG v3.1.0
  GIT_REPOSITORY git@git.greensocs.com:qemu/extra-components.git
  GIT_SHALLOW on
)
