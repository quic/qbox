set(LIBQEMU_TARGETS aarch64)

add_executable(platforms-vp
    ${PROJECT_SOURCE_DIR}/platforms/src/main.cc
)
if(APPLE)
    target_link_options(platforms-vp PRIVATE "LINKER:-dead_strip_dylibs")
else()
    target_link_options(platforms-vp PRIVATE "LINKER:-as-needed")
endif()

target_link_libraries(platforms-vp
    ${TARGET_LIBS}
    GreenSocs::qbox
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND EXISTS "/etc/lsb-release")
    file(STRINGS "/etc/lsb-release" LSB_RELEASE_CONTENT REGEX "DISTRIB_ID.*Ubuntu")
    if(LSB_RELEASE_CONTENT)
        message("The build is done from an Ubuntu machine.")
        add_custom_target(ubuntu
            COMMAND bash ${PROJECT_SOURCE_DIR}/platforms/ubuntu/fw/build_ubuntu_image.sh -s 4G -p xorg,openbox,mesa-utils,pciutils
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/platforms/ubuntu/fw
            COMMAND ${qbox_BINARY_DIR}/platforms/platforms-vp -l ${PROJECT_SOURCE_DIR}/platforms/ubuntu/conf.lua
            DEPENDS platforms-vp
        )
    else()
        message("The build is not done from an Ubuntu machine.")
    endif()
else()
    message("The build is done from a non-Linux system.")
endif()

add_subdirectory(cortex-m55-remote)