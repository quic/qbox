function top()
    local str = debug.getinfo(2, "S").source:sub(2)
    if str:match("(.*/)")
    then
        return str:match("(.*/)")
    else
        return "./"
    end
 end

dofile(top().."../../../configs/fw/utils.lua");

local QDSP6_CFG   = valid_file(top().."../../../configs/fw/qdsp6.lua")
dofile(QDSP6_CFG);

local NSP0_BASE     = 0x24000000 -- TURING_SS_0TURING
local NSP1_BASE     = 0x28000000 -- TURING_SS_1TURING
   -- ^^ This val is also described as 'TCM_BASE_ADDRESS' in
   -- Sys arch spec "TCM region memory map"
local TURING_SS_0TURING_QDSP6V68SS_CSR= 0x26380000
local TURING_SS_1TURING_QDSP6V68SS_CSR= 0x2A380000
-- The QDSP6v67SS private registers include CSR, L2VIC, QTMR registers.
-- The address base is determined by parameter QDSP6SS_PRIV_BASEADDR

-- Not used : local NSP0_AHB_LOW  = NSP0_BASE
-- Not used : local NSP0_TCM_BASE = NSP0_BASE
local NSP0_AHB_SIZE = 0x05000000
local NSP1_AHB_SIZE = 0x05000000
local NSP0_AHB_HIGH = NSP0_BASE + NSP0_AHB_SIZE -- = 0x1f000000
local NSP1_AHB_HIGH = NSP1_BASE + NSP1_AHB_SIZE -- = 0x25000000

local tbu0 = { topology_id=0x21C0,
              upstream_socket = { address=NSP0_AHB_HIGH,
                                  size=0xF00000000-NSP0_AHB_HIGH,
                                  relative_addresses=false
             }
            };
    -- Turing_NSP_1, ARID 14, AC_VM_CDSP_Q6_ELF:
local tbu1 = { topology_id=0x29C0,
             upstream_socket = { address=NSP1_AHB_HIGH,
                                 size=0xF00000000-NSP1_AHB_HIGH,
                                 relative_addresses=false
             }
            };

function get_remote_nspss(base, ahbs_base, cfgtable, start_addr, ahb_size, tbu)
    local cfgtable_base_addr = base + 0x180000;
    local sched_limit = true;
    return {
        hexagon_num_threads = 6;
        hexagon_thread_0={start_powered_off = false, start_halted=true, sched_limit=sched_limit};
        hexagon_thread_1={start_powered_off = true, sched_limit=sched_limit};
        hexagon_thread_2={start_powered_off = true, sched_limit=sched_limit};
        hexagon_thread_3={start_powered_off = true, sched_limit=sched_limit};
        hexagon_thread_4={start_powered_off = true, sched_limit=sched_limit};
        hexagon_thread_5={start_powered_off = true, sched_limit=sched_limit};
        HexagonQemuInstance = { tcg_mode="SINGLE",
            sync_policy = "multithread-unconstrained"};
        hexagon_start_addr = start_addr;
        l2vic={  mem           = {address=ahbs_base + 0x90000, size=0x1000};
                 fastmem       = {address=base     + 0x1e0000, size=0x10000}};
        qtimer={ mem           = {address=ahbs_base + 0xA0000, size=0x1000};
                 mem_view      = {address=ahbs_base + 0xA1000, size=0x2000}};
        cfgtable_base = cfgtable_base_addr;

        wdog  = { socket        = {address=ahbs_base + 0x84000, size=0x1000}};
        pll_0 = { socket        = {address=ahbs_base + 0x40000, size=0x10000}};
        pll_1 = { socket        = {address=base + 0x01001000, size=0x10000}};
        pll_2 = { socket        = {address=base + 0x01020000, size=0x10000}};
        pll_3 = { socket        = {address=base + 0x01021000, size=0x10000}};
        rom   = { target_socket = {address=cfgtable_base_addr, size=0x100 },
            read_only=true, load={data=cfgtable, offset=0}};

        csr = { socket = {address=ahbs_base, size=0x1000}};

        local_pass = {
                    target_socket_0 = {address=base , size= 0x01021000+0x10000, relative_addresses=false};
                    target_socket_1 = {address=ahbs_base , size= 0xA1000 + 0x2000, relative_addresses=false};
        };
        remote_exec_path="./vp-hex-remote";
        remote = {  
                    target_socket_0 = {address=tbu.upstream_socket.address, size= tbu.upstream_socket.size, relative_addresses=false};
                    target_socket_1 = {address=0x0, size=base, relative_addresses=false};
                    target_socket_2 = {address=ahbs_base + ahb_size, size=0xF00000000 - (ahbs_base + ahb_size) , relative_addresses=false};         
                 };
        fallback_memory = { target_socket={address=base, size=(ahbs_base + ahb_size) - base},
                 dmi_allow=false, verbose=true};
        num_initiators = 3;
        num_targets = 2;
        smmu500_tbu_0 = tbu;
    };
end

-- LeMans: NSP0 configuration --
local SA8775P_nsp0_config_table= get_SA8775P_nsp0_config_table();
local nsp0ss = get_remote_nspss(
        0x24000000, -- TURING_SS_0TURING
        0x26300000, -- TURING_SS_0TURING_QDSP6V68SS
        SA8775P_nsp0_config_table,
        0x9B800000, -- entry point address from bootimage_lemans.cdsp0.prodQ.pbn
        0x05000000, -- AHB_SIZE
        tbu0
        );
assert((SA8775P_nsp0_config_table[3] << 16) == TURING_SS_0TURING_QDSP6V68SS_CSR)
assert((SA8775P_nsp0_config_table[3] << 16) == TURING_SS_0TURING_QDSP6V68SS_CSR)
local NSP0_VTCM_BASE_ADDR = (SA8775P_nsp0_config_table[15] << 16)
local NSP0_VTCM_SIZE_BYTES = (SA8775P_nsp0_config_table[16] * 1024)


-- NSP1 configuration TODO : need ramdump of config_table for NSP1. --
local SA8775P_nsp1_config_table= get_SA8775P_nsp1_config_table();
local nsp1ss = get_remote_nspss(
        0x28000000, -- TURING_SS_1TURING
        0x2A300000, -- TURING_SS_1TURING_QDSP6V68SS
        SA8775P_nsp1_config_table,
        0x9D700000, -- entry point address from bootimage_lemans.cdsp1.prodQ.pbn
        0x05000000, -- AHB_SIZE
        tbu1
        );
assert((SA8775P_nsp1_config_table[11] << 16) == nsp1ss.l2vic.fastmem.address)
assert((SA8775P_nsp1_config_table[3] << 16) == TURING_SS_1TURING_QDSP6V68SS_CSR)
local NSP1_VTCM_BASE_ADDR = (SA8775P_nsp1_config_table[15] << 16)
local NSP1_VTCM_SIZE_BYTES = (SA8775P_nsp1_config_table[16] * 1024)

if (platform["hexagon_num_clusters"] ~= nil) then
    platform["hexagon_num_clusters"] = 0
end
if(platform["hexagon_ram_0"] ~= nil) then
    platform["hexagon_ram_0"] = {target_socket={address=NSP0_VTCM_BASE_ADDR, size=NSP0_VTCM_SIZE_BYTES}, shared_memory=true};
end
if(platform["hexagon_ram_1"] ~= nil) then
    platform["hexagon_ram_1"] = {target_socket={address=NSP1_VTCM_BASE_ADDR, size=NSP1_VTCM_SIZE_BYTES}, shared_memory=true};
end
if(platform["ipcc"]["irqs"] ~= nil) then
     platform["ipcc"]["irqs"] ={
        {irq=8, dst = "platform.gic.spi_in_229"},
        {irq=18, dst = "platform.remote_1.local_pass.target_signal_socket_0"}, -- NB relies on 2 clusters
        {irq=6,  dst = "platform.remote_0.local_pass.target_signal_socket_0"}
     }
end

tableMerge(platform, {
    remote_0 = nsp0ss,
    remote_1 = nsp1ss,
    });

local ram_nums = 5
for i=0,(ram_nums-1) do
    if (platform["ram_"..tostring(i)] ~= nil) then
        platform["ram_"..tostring(i)]["shared_memory"] = true
    end
end
