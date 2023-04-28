# qemu (unversioned)
CPMDeclarePackage(qemu
    NAME libqemu
    GIT_REPOSITORY ${GREENSOCS_GIT}qemu.git
    GIT_TAG libqemu-v7.0.50-v1.3.2
    GIT_SUBMODULES CMakeLists.txt
    GIT_SHALLOW on
)