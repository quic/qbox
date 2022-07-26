function get_SA8540P_nsp0_config_table()
 return {
    --/* captured from an SA840P-NSP0 via T32 */
        0x00001a00,   -- .l2tcm_base
        0x00000000,   -- .reserved
        0x00001b38,   -- .subsystem_base
        0x00001a19,   -- .etm_base
        0x00001a1a,   -- .l2cfg_base
        0x00001a1b,   -- .reserved2
        0x00001a80,   -- .l1s0_base
        0x00000000,   -- .axi2_lowaddr
        0x00001a1c,   -- .streamer_base
        0x00001a1d,   -- .clade_base
        0x00001a1e,   -- .fastl2vic_base
        0x00000080,   -- .jtlb_size_entries
        0x00000001,   -- .coproc_present
        0x00000004,   -- .ext_contexts
        0x00001a80,   -- .vtcm_base
        0x00002000,   -- .vtcm_size_kb
        0x00000400,   -- .l2tag_size
        0x00000400,   -- .l2ecomem_size
        0x0000003f,   -- .thread_enable_mask
        0x00001a1f,   -- .eccreg_base
        0x00000080,   -- .l2line_size
        0x00000000,   -- .tiny_core
        0x00000000,   -- .l2itcm_size
        0x00001a00,   -- .l2itcm_base
        0x00000000,   -- .clade2_base
        0x00000000,   -- .dtm_present
        0x00000001,   -- .dma_version
        0x00000007,   -- .hvx_vec_log_length
        0x00000000,   -- .core_id
        0x00000000,   -- .core_count
        0x00000040,   -- .hmx_int8_spatial
        0x00000020,   -- .hmx_int8_depth
        0x00000001,   -- .v2x_mode
        0x00000004,   -- .hmx_int8_rate
        0x00000020,   -- .hmx_fp16_spatial
        0x00000020,   -- .hmx_fp16_depth
        0x00000002,   -- .hmx_fp16_rate
        0x0000002e,   -- .hmx_fp16_acc_frac
        0x00000012,   -- .hmx_fp16_acc_int
        0x00000001,   -- .acd_preset
        0x00000001,   -- .mnd_preset
        0x00000010,   -- .l1d_size_kb
        0x00000020,   -- .l1i_size_kb
        0x00000002,   -- .l1d_write_policy
        0x00000040,   -- .vtcm_bank_width
        0x00000001,   -- .reserved3
        0x00000001,   -- .reserved4
        0x00000000,   -- .reserved5
        0x0000000a,   -- .hmx_cvt_mpy_size
        0x00000000,   -- .axi3_lowaddr
   };
end

function get_SA8540P_nsp1_config_table()
 return {
    --/* captured from an SA840P-NSP1 via T32 */
        0x00002000,
        0x00000000,
        0x00002138,
        0x00002019,
        0x0000201a,
        0x0000201b,
        0x00002080,
        0x00000000,
        0x0000201c,
        0x0000201d,
        0x0000201e,
        0x00000080,
        0x00000001,
        0x00000004,
        0x00002080,   -- .vtcm_base
        0x00002000,   -- .vtcm_size_kb
        0x00000400,
        0x00000400,
        0x0000003f,
        0x0000201f,
        0x00000080,
        0x00000000,
        0x00000000,
        0x00002000,
        0x00000000,
        0x00000000,
        0x00000001,
        0x00000007,
        0x00000000,
        0x00000000,
        0x00000040,
        0x00000020,
        0x00000001,
        0x00000004,
        0x00000020,
        0x00000020,
        0x00000002,
        0x0000002e,
        0x00000012,
        0x00000001,
        0x00000001,
        0x00000010,
        0x00000020,
        0x00000002,
        0x00000040,
        0x00000001,
        0x00000001,
        0x00000000,
        0x0000000a,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
     };
end
