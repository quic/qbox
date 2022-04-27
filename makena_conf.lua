-- Virtual platform configuration: Makena SoC, QNX



--[[
 smmu_service -v -H

smmu_test &
out32 0x15184010 0x10000
out32 0x15184008 0x10000
out32 0x15184000 0x10000
in32 0x15184020
out32 0x15184018 0x1
in32 0x15184020
in32 0x15184028
in32 0x1518402C
--]]
function top()
    local str = debug.getinfo(2, "S").source:sub(2)
    if str:match("(.*/)")
    then
        return str:match("(.*/)")
    else
        return "./"
    end
 end

print ("Lua config running. . . ");


local INITIAL_DDR_SPACE_14GB = 0x80000000
local OFFSET_MIFS_DDR_SPACE  = 0x20000000
local OFFSET_SMEM_DDR_SPACE  = 0x00900000
local DDR_SPACE_SIZE = 32*1024*1024*1024

local UNLIKELY_TO_BE_USED = INITIAL_DDR_SPACE_14GB + DDR_SPACE_SIZE
local DTB_LOAD_ADDR = UNLIKELY_TO_BE_USED + 512

local NSP0_BASE     = 0x1A000000 -- TURING_SS_0TURING
local NSP0_AHBS_BASE= 0x1B300000 -- TURING_SS_0TURING_QDSP6V68SS
                           -- TURING_SS_0TURING_QDSP6V68SS_PUB
-- The QDSP6v67SS private registers include CSR, L2VIC, QTMR registers.
-- The address base is determined by parameter QDSP6SS_PRIV_BASEADDR

local NSP0_AHB_LOW  = NSP0_BASE
local NSP0_TCM_BASE = NSP0_BASE
local NSP0_AHB_SIZE = 0x00500000
local NSP0_AHB_HIGH = NSP0_BASE + NSP0_AHB_SIZE

-- TURING_SS_0TURING_QDSP6SS_BOOT_CORE_START:
local NSP0_BOOT_CORE_START=NSP0_AHBS_BASE+0x400
-- TURING_SS_0TURING_QDSP6SS_BOOT_CMD:
-- NSP0_BOOT_CMD=NSP0_AHBS_BASE+0x404
-- TURING_SS_0TURING_QDSP6SS_BOOT_STATUS:
-- NSP0_BOOT_STATUS=NSP0_AHBS_BASE+0x408


local APSS_GIC600_GICD_APSS = 0x17A00000
local OFFSET_APSS_ALIAS0_GICR_CTLR = 0x60000

-- Makena VBSP, system UART addr: reallocated space at
--   PCIE_3APCIE_WRAPPER_AXI_G3X4_EDMA_AUTO:
local PCIE_3APCIE_WRAPPER_AXI_G3X4_EDMA_AUTO = 0x40000000
local UART0 = PCIE_3APCIE_WRAPPER_AXI_G3X4_EDMA_AUTO

local nsp0ss = {
    hexagon_num_threads = 1;
    hexagon_thread_0={start_powered_off = true};
--    hexagon_thread_1={start_powered_off = true};
--    hexagon_thread_2={start_powered_off = true};
--    hexagon_thread_3={start_powered_off = true};
    HexagonQemuInstance = { tcg_mode="SINGLE", sync_policy = "multithread-unconstrained"};
    hexagon_start_addr = 0x89F00000; -- entry point address from bootimage_makena.cdsp0.prodQ.pbn
    l2vic={  mem           = {address=NSP0_AHBS_BASE + 0x90000, size=0x1000};
             fastmem       = {address=NSP0_BASE     + 0x1e0000, size=0x10000}};
    qtimer={ mem           = {address=NSP0_AHBS_BASE + 0xA0000, size=0x1000};
             timer0_mem    = {address=NSP0_AHBS_BASE + 0xA1000, size=0x1000};
             timer1_mem    = {address=NSP0_AHBS_BASE + 0xA2000, size=0x1000}};
    pass = {target_socket  = {address=NSP0_AHB_LOW, size=NSP0_AHB_SIZE}};
};

platform = {
    arm_num_cpus = 8;
    num_redists = 1;
    hexagon_num_clusters = 1;
    hexagon_cluster_0 = nsp0ss;
    quantum_ns = 100000000;

    ArmQemuInstance = { tcg_mode="MULTI", sync_policy = "multithread-unconstrained"};

    ram=  {  target_socket = {address=INITIAL_DDR_SPACE_14GB, size=DDR_SPACE_SIZE}};
    hexagon_ram={target_socket={address=UNLIKELY_TO_BE_USED+0x0, size=0x08000000}};
    rom=  {  target_socket = {address=UNLIKELY_TO_BE_USED+0xde000000, size=0x400 },read_only=true};
    gic=  {  dist_iface    = {address=APSS_GIC600_GICD_APSS, size= OFFSET_APSS_ALIAS0_GICR_CTLR};
             redist_iface_0= {address=APSS_GIC600_GICD_APSS+OFFSET_APSS_ALIAS0_GICR_CTLR, size=0xf60000}};
    virtionet0= { mem    =   {address=0x1c120000, size=0x10000}, irq=18, netdev_str="type=user,hostfwd=tcp::2222-:22,hostfwd=tcp::2221-:21"};
    virtioblk0= { mem    =   {address=0x1c0d0000, size=0x2000}, irq=9, blkdev_str="file="..top().."fw/makena/images/system_qdrive.img,format=raw,if=none"};

    uart= {  simple_target_socket_0 = {address= UART0, size=0x1000}, irq=1};

    ipcc= {  socket        = {address=0x410000, size=0xfc000}, irq_8=229};

    smmu = { mem = {address=0x15000000, size=0x100000};
        num_tbu=2;
        num_pages=128;
        num_cb=128;
        tbu_sid_0 = 0x0;
        upstream_socket_0 = {address=0x0, size=0x100000000, relative_addresses=false};
        tbu_sid_1 = 0;
        upstream_socket_1 = {address=0x0, size=0xd81e0000, relative_addresses=false};
        irq_context = 103;
        irq_global = 65;
    };
    qtb = { control_socket = {address=0x15180000, size=0x80000}};

    fallback_memory = { target_socket={address=0x0, size=0x40000000}, dmi_allow=false, verbose=true, load={csv_file=top().."fw/makena/SA8540P_MakenaAU_v2_Registers.csv", offset=0, addr_str="Address", value_str="Reset Value", byte_swap=true}};
    load={
        {bin_file=top().."fw/makena/images/bl31.bin",
         address=INITIAL_DDR_SPACE_14GB};
        {bin_file=top().."fw/makena/images/mifs_qdrive_qvp.img",
         address=INITIAL_DDR_SPACE_14GB + OFFSET_MIFS_DDR_SPACE };
        {bin_file=top().."fw/makena/images/smem_v3.bin",
         address=INITIAL_DDR_SPACE_14GB + OFFSET_SMEM_DDR_SPACE };
        {elf_file=top().."fw/hexagon-images/bootimage_makena.cdsp0.prodQ.pbn"};
    };
    --uart_backend_port=4001;
};


if (platform.arm_num_cpus > 0) then
    for i=0,(platform.arm_num_cpus-1) do
        local cpu = {
            has_el3 = true;
            has_el2 = false;
            psci_conduit = "smc";
            mp_affinity = (math.floor(i / 8) << 8) | (i % 8);
            start_powered_off = true;
        };
        if (i==0) then
            cpu["rvbar"] = INITIAL_DDR_SPACE_14GB;
            cpu["start_powered_off"] = false;
        end
        platform["cpu_"..tostring(i)]=cpu;
    end
end


if (platform.hexagon_num_clusters > 0) then
    platform["cfgTable"] = {
        fastl2vic_base = platform.hexagon_cluster_0.l2vic.fastmem.address,
    };

    platform["cfgExtensions"] = {
        cfgtable_size = platform.rom.target_socket.size,
        l2vic_base = platform.hexagon_cluster_0.l2vic.mem.address,
        qtmr_rg0 = platform.hexagon_cluster_0.qtimer.timer0_mem.address,
        qtmr_rg1 = platform.hexagon_cluster_0.qtimer.timer1_mem.address,
    };
end

print ("Lua config run. . . ");
