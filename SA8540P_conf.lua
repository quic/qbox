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

 function valid_file(file)
    local f = io.open(file, "r")
    if (f == nil)
    then
        print ("ERROR:")
        print (file.." Not found.")
        os.exit(1)
    end
    io.close(f);
    return file
 end

print ("Lua config running. . . ");


local INITIAL_DDR_SPACE_14GB = 0x80000000
local OFFSET_MIFS_DDR_SPACE  = 0x20000000
local OFFSET_SMEM_DDR_SPACE  = 0x00900000
local DDR_SPACE_SIZE = 16*1024*1024*1024

if os.getenv("QQVP_IMAGE_DIR") == nil then
    SYSTEM_QDRIVE     = valid_file(top().."fw/makena/images/system_qdrive_qvp.img")
    MIFS_QDRIVE       = valid_file(top().."fw/makena/images/mifs_qdrive_qvp.img")
    QURT_CDSP0        = valid_file(top().."fw/hexagon-images/bootimage_relocflag_withdummyseg.cdsp0.prodQ.pbn")
    QURT_CDSP1        = valid_file(top().."fw/hexagon-images/bootimage_relocflag_withdummyseg.cdsp1.prodQ.pbn")
    BL31_BIN          = valid_file(top().."fw/makena/images/bl31.bin")
    SMEM_MAKENA_BIN   = valid_file(top().."fw/makena/images/smem_makena.bin")
    CMD_DB_HEADER_BIN = valid_file(top().."fw/makena/images/cmd_db_header.bin")
    CMD_DB_BIN        = valid_file(top().."fw/makena/images/cmd_db.bin")
else
    IMAGE_DIR = os.getenv("QQVP_IMAGE_DIR").."/"
    SYSTEM_QDRIVE     = valid_file(IMAGE_DIR.."system_qdrive_qvp.img")
    MIFS_QDRIVE       = valid_file(IMAGE_DIR.."mifs_qdrive_qvp.img")
    QURT_CDSP0        = valid_file(IMAGE_DIR.."bootimage_relocflag_withdummyseg_makena.cdsp0.prodQ.pbn")
    QURT_CDSP1        = valid_file(IMAGE_DIR.."bootimage_relocflag_withdummyseg_makena.cdsp1.prodQ.pbn")
    BL31_BIN          = valid_file(IMAGE_DIR.."bl31.bin")
    SMEM_MAKENA_BIN   = valid_file(IMAGE_DIR.."smem_makena.bin")
    CMD_DB_HEADER_BIN = valid_file(IMAGE_DIR.."cmd_db_header.bin")
    CMD_DB_BIN        = valid_file(IMAGE_DIR.."cmd_db.bin")
end

-- The following are in GIT and probably don't need to be validated.
MAKENA_REGS_CSV = top().."fw/makena/SA8540P_MakenaAU_v2_Registers.csv"
HEX_CFGTABLES   = top().."fw/hex_cfgtables.lua"

local NSP0_BASE     = 0x1A000000 -- TURING_SS_0TURING
local NSP1_BASE     = 0x20000000 -- TURING_SS_1TURING
   -- ^^ This val is also described as 'TCM_BASE_ADDRESS' in
   -- Sys arch spec "TCM region memory map"
local NSP0_AHBS_BASE= 0x1B300000 -- TURING_SS_0TURING_QDSP6V68SS
local NSP1_AHBS_BASE= 0x21300000 -- TURING_SS_0TURING_QDSP6V68SS
local TURING_SS_0TURING_QDSP6V68SS_CSR= 0x1B380000
local TURING_SS_1TURING_QDSP6V68SS_CSR= 0x21380000
-- The QDSP6v67SS private registers include CSR, L2VIC, QTMR registers.
-- The address base is determined by parameter QDSP6SS_PRIV_BASEADDR

-- Not used : local NSP0_AHB_LOW  = NSP0_BASE
-- Not used : local NSP0_TCM_BASE = NSP0_BASE
local NSP0_AHB_SIZE = 0x05000000
local NSP1_AHB_SIZE = 0x05000000
local NSP0_AHB_HIGH = NSP0_BASE + NSP0_AHB_SIZE -- = 0x1f000000
local NSP1_AHB_HIGH = NSP1_BASE + NSP1_AHB_SIZE -- = 0x25000000

local IPC_ROUTER_TOP = 0x00400000

-- TURING_SS_0TURING_QDSP6SS_BOOT_CORE_START:
-- Not used: local NSP0_BOOT_CORE_START=NSP0_AHBS_BASE+0x400
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
-- local UART0 = 0x10000000

local TCSR_SOC_HW_VERSION_ADDR = 0x1FC8000;
local TCSR_SOC_EMULATION_TYPE_ADDR = TCSR_SOC_HW_VERSION_ADDR+4;
local SMEM_TCSR_TZ_WONCE_ADDR=0x01fd4000;
local SMEM_TARG_INFO_ADDR=0x80aff2c0; -- must correspond to smem_v3.bin
local RPMH_PDC_COMPUTE_PDC_PARAM_RESOURCE_DRVd = 0xb2c1004;
local RPMH_PDC_COMPUTE_PDC_PARAM_SEQ_CONFIG_DRVd = 0xb2c1008;
local RPMH_PDC_NSP_PDC_PARAM_RESOURCE_DRVd = 0xb2d1004;
local RPMH_PDC_NSP_PDC_PARAM_SEQ_CONFIG_DRVd = 0xb2d1008;
local TURING_SS_0TURING_QDSP6SS_RSCC_RSC_PARAM_RSC_CONFIG_DRVd = 0x1b3b0008;
local TURING_SS_0TURING_RSCC_RSC_PARAM_RSC_CONFIG_DRVd = 0x1b0a4008
local TURING_SS_1TURING_QDSP6SS_RSCC_RSC_PARAM_RSC_CONFIG_DRVd = 0x213b0008;
local TURING_SS_1TURING_RSCC_RSC_PARAM_RSC_CONFIG_DRVd = 0x210a4008;

dofile (top().."fw/hex_cfgtables.lua")

function get_nspss(base, ahbs_base, cfgtable, start_addr, ahb_size)
    local cfgtable_base_addr = base + 0x180000;
    return {
        hexagon_num_threads = 6;
        hexagon_thread_0={start_powered_off = false, start_halted=true};
        hexagon_thread_1={start_powered_off = true};
        hexagon_thread_2={start_powered_off = true};
        hexagon_thread_3={start_powered_off = true};
        hexagon_thread_4={start_powered_off = true};
        hexagon_thread_5={start_powered_off = true};
        HexagonQemuInstance = { tcg_mode="SINGLE",
            sync_policy = "multithread-unconstrained"};
        hexagon_start_addr = start_addr;
        l2vic={  mem           = {address=ahbs_base + 0x90000, size=0x1000};
                 fastmem       = {address=base     + 0x1e0000, size=0x10000}};
        qtimer={ mem           = {address=ahbs_base + 0xA0000, size=0x1000};
                 mem_view      = {address=ahbs_base + 0xA1000, size=0x2000}};
        pass = {target_socket  = {address=0x0 , size=base + ahb_size,
            relative_addresses=false}};
        cfgtable_base = cfgtable_base_addr;

        wdog  = { socket        = {address=ahbs_base + 0x84000, size=0x1000}};
        pll_0 = { socket        = {address=ahbs_base + 0x40000, size=0x10000}};
        pll_1 = { socket        = {address=base + 0x01001000, size=0x10000}};
        pll_2 = { socket        = {address=base + 0x01020000, size=0x10000}};
        pll_3 = { socket        = {address=base + 0x01021000, size=0x10000}};
        rom   = { target_socket = {address=cfgtable_base_addr, size=0x100 },
            read_only=true, load={data=cfgtable, offset=0}};

        csr = { socket = {address=ahbs_base, size=0x1000}};
    };
end


-- NSP0 configuration --
local SA8540P_nsp0_config_table= get_SA8540P_nsp0_config_table();
local nsp0ss = get_nspss(
        0x1A000000, -- TURING_SS_0TURING
        0x1B300000, -- TURING_SS_0TURING_QDSP6V68SS
        SA8540P_nsp0_config_table,
        0x89F00000, -- entry point address from bootimage_makena.cdsp0.prodQ.pbn
        0x05000000 -- AHB_SIZE
        );

assert((SA8540P_nsp0_config_table[11] << 16) == nsp0ss.l2vic.fastmem.address)
-- So far, nothing depends on _csr, but a good sanity check:
assert((SA8540P_nsp0_config_table[3] << 16) == TURING_SS_0TURING_QDSP6V68SS_CSR)
local NSP0_VTCM_BASE_ADDR = (SA8540P_nsp0_config_table[15] << 16)
local NSP0_VTCM_SIZE_BYTES = (SA8540P_nsp0_config_table[16] * 1024)

-- NSP1 configuration --
local SA8540P_nsp1_config_table= get_SA8540P_nsp1_config_table();
local nsp1ss = get_nspss(
        0x20000000, -- TURING_SS_1TURING
        0x21300000, -- TURING_SS_1TURING_QDSP6V68SS
        SA8540P_nsp1_config_table,
        0x8C600000, -- entry point address from bootimage_makena.cdsp1.prodQ.pbn
        0x05000000 -- AHB_SIZE
        );
assert((SA8540P_nsp1_config_table[11] << 16) == nsp1ss.l2vic.fastmem.address)
assert((SA8540P_nsp1_config_table[3] << 16) == TURING_SS_1TURING_QDSP6V68SS_CSR)
local NSP1_VTCM_BASE_ADDR = (SA8540P_nsp1_config_table[15] << 16)
local NSP1_VTCM_SIZE_BYTES = (SA8540P_nsp1_config_table[16] * 1024)



platform = {
    arm_num_cpus = 8;
    num_redists = 1;

    hexagon_num_clusters = 2;
    hexagon_cluster_0 = nsp0ss;
    hexagon_cluster_1 = nsp1ss;
    quantum_ns = 10000000;

    ArmQemuInstance = { tcg_mode="MULTI", sync_policy = "multithread-unconstrained"};

    ram_0=  {  target_socket = {address=INITIAL_DDR_SPACE_14GB, size=DDR_SPACE_SIZE/2}};
    ram_1=  {  target_socket = {address=INITIAL_DDR_SPACE_14GB+(DDR_SPACE_SIZE/2), size=DDR_SPACE_SIZE/2}};
    hexagon_ram_0={target_socket={address=NSP0_VTCM_BASE_ADDR, size=NSP0_VTCM_SIZE_BYTES}};
    hexagon_ram_1={target_socket={address=NSP1_VTCM_BASE_ADDR, size=NSP1_VTCM_SIZE_BYTES}};
    gic=  {  dist_iface    = {address=APSS_GIC600_GICD_APSS, size= OFFSET_APSS_ALIAS0_GICR_CTLR};
             redist_iface_0= {address=APSS_GIC600_GICD_APSS+OFFSET_APSS_ALIAS0_GICR_CTLR, size=0x1C0000}};
    virtionet0= { mem    =   {address=0x1c120000, size=0x10000}, irq=18, netdev_str="type=user,hostfwd=tcp::2222-:22,hostfwd=tcp::2221-:21"};
    virtioblk0= { mem    =   {address=0x1c0d0000, size=0x2000}, irq=9, blkdev_str="file="..SYSTEM_QDRIVE..",format=raw,if=none"};

    qtimer_0=   { mem     =   {address=0x17c20000, size=0x1000};
                  mem_view=   {address=0x17c21000, size=0x1000*2*7}; -- 0x1000*nr_frames*nr_views
                  irq0=40;irq1=41;irq2=42;irq3=43;irq4=44;irq5=45;irq6=46;
                  nr_frames=7;nr_views=2;cnttid=0x1111515};
    uart= {  simple_target_socket_0 = {address= UART0, size=0x1000}, irq=1};

    ipcc= {  socket        = {address=IPC_ROUTER_TOP, size=0xfc000},
             irqs = {
                {irq=8, dst = "platform.gic.spi_in_229"},
                {irq=18, dst = "platform.hexagon_cluster_1.l2vic.irq_in_30"},
                {irq=6,  dst = "platform.hexagon_cluster_0.l2vic.irq_in_30"},
            }
        };

    qtb = { control_socket = {address=0x15180000, size=0x80000}};

    smmu = { socket = {address=0x15000000, size=0x100000};
             mem = {address=0x15000000, size=0x100000};
             num_tbu=2;
             num_pages=128;
             num_cb=128;
             irq_context = 103;
             irq_global = 65;
           };

    tbu_0 = { topology_id=0x31A0,
              upstream_socket = { topology_id=0x31A0,
                                  address=NSP0_AHB_HIGH,
                                  size=0xF00000000-NSP0_AHB_HIGH,
                                  relative_addresses=false
             }
            };
    tbu_1 = { topology_id=0x39A0,
             upstream_socket = { topology_id=0x39A0,
                                 address=NSP1_AHB_HIGH,
                                 size=0xF00000000-NSP1_AHB_HIGH,
                                 relative_addresses=false
             }
            };

    fallback_memory = { target_socket={address=0x0, size=0x40000000},
                        dmi_allow=false, verbose=true,
                        load={csv_file=MAKENA_REGS_CSV,
                        offset=0, addr_str="Address",
                        value_str="Reset Value", byte_swap=true}
                      };
    load={
        {bin_file=BL31_BIN,
         address=INITIAL_DDR_SPACE_14GB};
        {bin_file=MIFS_QDRIVE,
         address=INITIAL_DDR_SPACE_14GB + OFFSET_MIFS_DDR_SPACE };
        {bin_file=SMEM_MAKENA_BIN,
         address=INITIAL_DDR_SPACE_14GB + OFFSET_SMEM_DDR_SPACE };
        {bin_file=CMD_DB_HEADER_BIN,
         address= 0x0C3F0000};
        {bin_file=CMD_DB_BIN,
         address= 0x80860000};
	    {elf_file=QURT_CDSP0};
	    {elf_file=QURT_CDSP1};
        {data={0x60140200}, address=TCSR_SOC_HW_VERSION_ADDR};
        {data={SMEM_TARG_INFO_ADDR}, address=SMEM_TCSR_TZ_WONCE_ADDR};
        {data={0x00005381},
         address=RPMH_PDC_COMPUTE_PDC_PARAM_RESOURCE_DRVd};
        {data={0x00180411},
         address=RPMH_PDC_COMPUTE_PDC_PARAM_SEQ_CONFIG_DRVd};
        {data={0x00005381},
         address=RPMH_PDC_NSP_PDC_PARAM_RESOURCE_DRVd};
        {data={0x00180411},
         address=RPMH_PDC_NSP_PDC_PARAM_SEQ_CONFIG_DRVd};
        {data={0x01300214},
         address=TURING_SS_0TURING_QDSP6SS_RSCC_RSC_PARAM_RSC_CONFIG_DRVd};
        {data={0x01180214},
         address=TURING_SS_0TURING_RSCC_RSC_PARAM_RSC_CONFIG_DRVd};
        {data={0x01300214},
         address=TURING_SS_1TURING_QDSP6SS_RSCC_RSC_PARAM_RSC_CONFIG_DRVd};
        {data={0x01180214},
         address=TURING_SS_1TURING_RSCC_RSC_PARAM_RSC_CONFIG_DRVd};


--      {data={0x00020400}, address=0x10E0000};
--      {data={0x60000003}, address=0x10E000C};
--      {data={0x0}, address=TCSR_SOC_EMULATION_TYPE_ADDR}; -- 0=silicon
--      {data={0x1}, address=TCSR_SOC_EMULATION_TYPE_ADDR}; -- 1=RUMI
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

print ("Lua config run. . . ");
