gs_create_dymod(smmu500)

target_link_libraries(smmu500 PUBLIC
    gs_memory
    router
    reg_router
)
