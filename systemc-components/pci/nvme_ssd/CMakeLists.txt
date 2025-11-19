gs_create_dymod(nvme_ssd)

target_sources(
    nvme_ssd PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/nvme_ssd.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/bitset.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/config.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/def.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/disk.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/dma.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/interface.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/namespace.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/pci_device_base.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/queue.cc
)

target_include_directories(
    nvme_ssd PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/systemc-components/pci/gs_gpex/include>
)