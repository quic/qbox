
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
function get_SA8775P_nsp0_config_table()
 return {
    -- /* captured from email from Aaron Garofano <garofano@qti.qualcomm.com> */
        0x00002400, -- 000000 .l2tcm_base
        0x00000000, -- 000004 .reserved
        0x00002638, -- 000008 .subsystem_base
        0x00002419, -- 00000c .etm_base
        0x0000241a, -- 000010 .l2cfg_base
        0x0000241b, -- 000014 .reserved2
        0x00002500, -- 000018 .l1s0_base
        0x00000000, -- 00001c .axi2_lowaddr
        0x00000000, -- 000020 .streamer_base
        0x00000000, -- 000024 .clade_base
        0x0000241e, -- 000028 .fastl2vic_base
        0x00000080, -- 00002c .jtlb_size_entries
        0x00000001, -- 000030 .coproc_present
        0x00000004, -- 000034 .ext_contexts
        0x00002500, -- 000038 .vtcm_base
        0x00002000, -- 00003c .vtcm_size_kb
        0x00000400, -- 000040 .l2tag_size
        0x00000000, -- 000044 .l2ecomem_size
        0x0000003f, -- 000048 .thread_enable_mask
        0x0000241f, -- 00004c .eccreg_base
        0x00000080, -- 000050 .l2line_size
        0x00000000, -- 000054 .tiny_core
        0x00000000, -- 000058 .l2itcm_size
        0x00002400, -- 00005c .l2itcm_base
        0x00000000, -- 000060 .clade2_base
        0x00000000, -- 000064 .dtm_present
        0x00000003, -- 000068 .dma_version
        0x00000007, -- 00006c .hvx_vec_log_length
        0x00000000, -- 000070 .core_id
        0x00000000, -- 000074 .core_count
        0x00000040, -- 000078 .hmx_int8_spatial
        0x00000020, -- 00007c .hmx_int8_depth
        0x00000001, -- 000080 .v2x_mode
        0x00000008, -- 000084 .hmx_int8_rate
        0x00000020, -- 000088 .hmx_fp16_spatial
        0x00000000, -- 00008c .hmx_fp16_depth
        0x00000002, -- 000090 .hmx_fp16_rate
        0x00000016, -- 000094 .hmx_fp16_acc_frac
        0x00000006, -- 000098 .hmx_fp16_acc_int
        0x00000001, -- 00009c .acd_preset
        0x00000000, -- 0000a0 .mnd_preset
        0x00000010, -- 0000a4 .l1d_size_kb
        0x00000020, -- 0000a8 .l1i_size_kb
        0x00000002, -- 0000ac .l1d_write_policy
        0x00000080, -- 0000b0 .vtcm_bank_width
        0x00000001, -- 0000b4 .atomics_version
        0x00000000, -- 0000b8 .reserved4
        0x00000003, -- 0000bc .victim_cache_mode
        0x0000000a, -- 0000c0 .hmx_cvt_mpy_size
        0x000000e0, -- 0000c4 .axi3_lowaddr
        0x00000080, -- 0000c8 .capacity_domain
        0x00000000, -- 0000cc .reserved
        0x00000002, -- 0000d0 .hmx_int8_subcolumns
        0x00000005, -- 0000d4 .corecfg_present
        0x00000007, -- 0000d8 .hmx_fp16_acc_exp
        0x00000000, -- 0000dc .AXIM2_secondary_base
        0x00000000, -- 0000e0 .reserved
};
end

function get_SA8775P_nsp1_config_table()
 return {
    -- /* captured from email from Aaron Garofano <garofano@qti.qualcomm.com> */
        0x00002800, -- 000000 .l2tcm_base
        0x00000000, -- 000004 .reserved
        0x00002a38, -- 000008 .subsystem_base
        0x00002819, -- 00000c .etm_base
        0x0000281a, -- 000010 .l2cfg_base
        0x0000281b, -- 000014 .reserved2
        0x00002900, -- 000018 .l1s0_base
        0x00000000, -- 00001c .axi2_lowaddr
        0x00000000, -- 000020 .streamer_base
        0x00000000, -- 000024 .clade_base
        0x0000281e, -- 000028 .fastl2vic_base
        0x00000080, -- 00002c .jtlb_size_entries
        0x00000001, -- 000030 .coproc_present
        0x00000004, -- 000034 .ext_contexts
        0x00002900, -- 000038 .vtcm_base
        0x00002000, -- 00003c .vtcm_size_kb
        0x00000400, -- 000040 .l2tag_size
        0x00000400, -- 000044 .l2ecomem_size
        0x0000003f, -- 000048 .thread_enable_mask
        0x0000281f, -- 00004c .eccreg_base
        0x00000080, -- 000050 .l2line_size
        0x00000000, -- 000054 .tiny_core
        0x00000000, -- 000058 .l2itcm_size
        0x00002800, -- 00005c .l2itcm_base
        0x00000000, -- 000060 .clade2_base
        0x00000000, -- 000064 .dtm_present
        0x00000003, -- 000068 .dma_version
        0x00000007, -- 00006c .hvx_vec_log_length
        0x00000000, -- 000070 .core_id
        0x00000000, -- 000074 .core_count
        0x00000040, -- 000078 .hmx_int8_spatial
        0x00000020, -- 00007c .hmx_int8_depth
        0x00000001, -- 000080 .v2x_mode
        0x00000008, -- 000084 .hmx_int8_rate
        0x00000020, -- 000088 .hmx_fp16_spatial
        0x00000020, -- 00008c .hmx_fp16_depth
        0x00000002, -- 000090 .hmx_fp16_rate
        0x00000016, -- 000094 .hmx_fp16_acc_frac
        0x00000006, -- 000098 .hmx_fp16_acc_int
        0x00000001, -- 00009c .acd_preset
        0x00000001, -- 0000a0 .mnd_preset
        0x00000010, -- 0000a4 .l1d_size_kb
        0x00000020, -- 0000a8 .l1i_size_kb
        0x00000002, -- 0000ac .l1d_write_policy
        0x00000080, -- 0000b0 .vtcm_bank_width
        0x00000001, -- 0000b4 .atomics_version
        0x00000001, -- 0000b8 .reserved4
        0x00000003, -- 0000bc .victim_cache_mode
        0x0000000a, -- 0000c0 .hmx_cvt_mpy_size
        0x000000e0, -- 0000c4 .axi3_lowaddr
        0x00000080, -- 0000c8 .capacity_domain
        0x00000000, -- 0000cc .reserved
        0x00000002, -- 0000d0 .hmx_int8_subcolumns
        0x00000005, -- 0000d4 .corecfg_present
        0x00000007, -- 0000d8 .hmx_fp16_acc_exp
        0x00000000, -- 0000dc .AXIM2_secondary_base
        0x00000000, -- 0000e0 .reserved
};
end

-- From the DSP Silicon Validation team
function get_SA8775P_adsp_config_table()
 return {
        0x000002C0, -- L2TCM
        0x00000000, -- Reserved
        0x00000308, -- Subsystem_base
        0x000002F9, -- ETM_cfg_base
        0x000002FA, -- L2_cfg_base
        0x000002FB, -- Coproc_reserved
        0x000002E0, -- Coproc_reserved
        0x00000390, -- AXIM2_base
        0x000002FC, -- Stream_cfg_base
        0x000002FD, -- CLADE_cfg_base
        0x000002FE, -- Fastl2vic_base
        0x000000C0, -- JTLB_size
        0x00000000, -- Coproc_type
        0x00000000, -- Coproc_contexts
        0x00000000, -- VTCM_base
        0x00000000, -- VTCM_size
        0x00000400, -- L2_tag_size
        0x00000800, -- L2_array_size
        0x0000000F, -- Thread_enable_mask
        0x000002FF, -- ECC_cfg_base
        0x00000040, -- L2_line_size
        0x00000000, -- Audio_extension
 };
end

function _get_dsp(dsp_type, arch, base, ahbs_base, cfgtable, start_addr, ahb_size,
                  num_threads)
    local offset = 0
    if dsp_type == "adsp" then
        -- ADSP addresses are offset
        offset = 0x200000
    end
    local cfgtable_base_addr = base + offset + 0x180000
    local sched_limit = true;
    assert(num_threads >= 1);
    local dsp = {
        hexagon_num_threads = num_threads;
        isdben_secure  = true;
        isdben_trusted = true;
        hexagon_thread_0={ start_powered_off = false,
            start_halted=true,
            sched_limit=sched_limit,
            dsp_arch=arch,
        };
        HexagonQemuInstance = { tcg_mode="SINGLE",
            sync_policy = "multithread-unconstrained"};
        hexagon_start_addr = start_addr;
        l2vic={  mem           = {address=ahbs_base + offset + 0x90000, size=0x1000};
                 fastmem       = {address=base      + offset + 0x1e0000, size=0x10000}};
        qtimer={ mem           = {address=ahbs_base + offset + 0xA0000, size=0x1000};
                 mem_view      = {address=ahbs_base + offset + 0xA1000, size=0x2000}};
        pass = {target_socket  = {address=0x0 , size=base + offset + ahb_size,
            relative_addresses=false}};
        cfgtable_base = cfgtable_base_addr;

        wdog  = { socket        = {address=ahbs_base + offset + 0x84000,    size=0x1000}};
        pll_0 = { socket        = {address=ahbs_base + offset + 0x40000,    size=0x10000}};
        rom   = { target_socket = {address=cfgtable_base_addr, size=0x100 },
            read_only=true, load={data=cfgtable, offset=0}};

        csr = { socket = {address=ahbs_base + offset, size=0x1000}};
    };
    for th=1,num_threads-1 do
      dsp["hexagon_thread_" .. th] = {
          start_powered_off = true,
          sched_limit=sched_limit,
          dsp_arch=arch,
      };
    end
    return dsp
end

function get_dsp(arch, base, ahbs_base, cfgtable, start_addr, ahb_size,
                 num_threads)
    return _get_dsp("cdsp", arch, base, ahbs_base, cfgtable, start_addr,
                    ahb_size, num_threads)
end

function get_adsp(arch, base, ahbs_base, cfgtable, start_addr, ahb_size,
                 num_threads)
    return _get_dsp("adsp", arch, base, ahbs_base, cfgtable, start_addr,
                    ahb_size, num_threads)
end
