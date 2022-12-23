-- Virtual platform configuration

function top()
    local str = debug.getinfo(2, "S").source:sub(2)
    if str:match("(.*/)")
    then
        return str:match("(.*/)")
    else
        return "./"
    end
 end

dofile(top().."/fw/utils.lua");
print ("Lua config running. . . ");

-- these values are helpful for local configuration, so make them global
 INITIAL_DDR_SPACE_14GB = 0x80000000
 OFFSET_MIFS_DDR_SPACE  = 0x60000000
 OFFSET_SMEM_DDR_SPACE  = 0x10900000
 DDR_SPACE_SIZE = 16*1024*1024*1024



local MAKENA_REGS_CSV = valid_file(top().."fw/makena/SA8540P_MakenaAU_v2_Registers.csv")
local QDSP6_CFG   = valid_file(top().."fw/qdsp6.lua")

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

local IPC_ROUTER_TOP = 0x00400000


local APSS_GIC600_GICD_APSS = 0x17A00000
local OFFSET_APSS_ALIAS0_GICR_CTLR = 0x60000

-- Makena VBSP, system UART addr: reallocated space at
--   PCIE_3APCIE_WRAPPER_AXI_G3X4_EDMA_AUTO:
local PCIE_3APCIE_WRAPPER_AXI_G3X4_EDMA_AUTO = 0x40000000
local UART0 = PCIE_3APCIE_WRAPPER_AXI_G3X4_EDMA_AUTO
-- local UART0 = 0x10000000

-- QUPv3 uart
local QUPV3_1_SE2 = 0x00A88000

local TCSR_SOC_HW_VERSION_ADDR = 0x1FC8000;
local TCSR_SOC_EMULATION_TYPE_ADDR = TCSR_SOC_HW_VERSION_ADDR+4;
local SMEM_TCSR_TZ_WONCE_ADDR=0x01fd4000;
local SMEM_TARG_INFO_ADDR=0x90aff320; -- must correspond to smem*.bin
local RPMH_PDC_COMPUTE_PDC_PARAM_RESOURCE_DRVd = 0xb2c1004;
local RPMH_PDC_COMPUTE_PDC_PARAM_SEQ_CONFIG_DRVd = 0xb2c1008;
local RPMH_PDC_NSP_PDC_PARAM_RESOURCE_DRVd = 0xb2f1004;
local RPMH_PDC_NSP_PDC_PARAM_SEQ_CONFIG_DRVd = 0xb2d1008;
local TURING_SS_0TURING_QDSP6SS_RSCC_RSC_PARAM_RSC_CONFIG_DRVd = 0x20CB0008;
local TURING_SS_0TURING_RSCC_RSC_PARAM_RSC_CONFIG_DRVd = 0x208A4008;
local TURING_SS_1TURING_QDSP6SS_RSCC_RSC_PARAM_RSC_CONFIG_DRVd = 0x263B0008;
local TURING_SS_1TURING_RSCC_RSC_PARAM_RSC_CONFIG_DRVd = 0x260A4008;
local HWKM_MASTER_CFG_KM_RNG_EE10_PRNG_DATA_OUT	= 0x10da000;
local HWKM_MASTER_CFG_KM_RNG_EE10_PRNG_STATUS= 0x10da004;

dofile(QDSP6_CFG);

-- LeMans: NSP0 configuration --
local SA8775P_nsp0_config_table= get_SA8775P_nsp0_config_table();
local nsp0ss = get_nspss(
        0x24000000, -- TURING_SS_0TURING
        0x26300000, -- TURING_SS_0TURING_QDSP6V68SS
        SA8775P_nsp0_config_table,
        0x9B800000, -- entry point address from bootimage_lemans.cdsp0.prodQ.pbn
        0x05000000 -- AHB_SIZE
        );
assert((SA8775P_nsp0_config_table[3] << 16) == TURING_SS_0TURING_QDSP6V68SS_CSR)
assert((SA8775P_nsp0_config_table[3] << 16) == TURING_SS_0TURING_QDSP6V68SS_CSR)
local NSP0_VTCM_BASE_ADDR = (SA8775P_nsp0_config_table[15] << 16)
local NSP0_VTCM_SIZE_BYTES = (SA8775P_nsp0_config_table[16] * 1024)


-- NSP1 configuration TODO : need ramdump of config_table for NSP1. --
local SA8775P_nsp1_config_table= get_SA8775P_nsp1_config_table();
local nsp1ss = get_nspss(
        0x28000000, -- TURING_SS_1TURING
        0x2A300000, -- TURING_SS_1TURING_QDSP6V68SS
        SA8775P_nsp1_config_table,
        0x9D700000, -- entry point address from bootimage_lemans.cdsp1.prodQ.pbn
        0x05000000 -- AHB_SIZE
        );
assert((SA8775P_nsp1_config_table[11] << 16) == nsp1ss.l2vic.fastmem.address)
assert((SA8775P_nsp1_config_table[3] << 16) == TURING_SS_1TURING_QDSP6V68SS_CSR)
local NSP1_VTCM_BASE_ADDR = (SA8775P_nsp1_config_table[15] << 16)
local NSP1_VTCM_SIZE_BYTES = (SA8775P_nsp1_config_table[16] * 1024)




platform = {
    arm_num_cpus = 8;
    num_redists = 1;
    with_gpu = false;

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
    virtionet0= { mem    =   {address=0x1c120000, size=0x10000}, irq=18, netdev_str="type=user,hostfwd=tcp::2222-:22,hostfwd=tcp::2221-:21,hostfwd=tcp::56283-:56283,hostfwd=tcp::65534-:65534,hostfwd=tcp::65535-:65535"};
--    virtioblk_0= { mem    =   {address=0x1c0d0000, size=0x2000}, irq=9, blkdev_str="file="..SYSTEM_QDRIVE..",format=raw,if=none"};

    qtimer_0=   { mem     =   {address=0x17c20000, size=0x1000};
                  mem_view=   {address=0x17c21000, size=0x1000*2*7}; -- 0x1000*nr_frames*nr_views
                  irq0=40;irq1=41;irq2=42;irq3=43;irq4=44;irq5=45;irq6=46;
                  nr_frames=7;nr_views=2;cnttid=0x1111515};
    uart= {  simple_target_socket_0 = {address= UART0, size=0x1000}, irq=379};
    uart_qup= {  simple_target_socket_0 = {address=QUPV3_1_SE2, size=0x2000}, irq=355};

    ipcc= {  socket        = {address=IPC_ROUTER_TOP, size=0xfc000},
             irqs = {
                {irq=8, dst = "platform.gic.spi_in_229"},
                {irq=18, dst = "platform.hexagon_cluster_1.l2vic.irq_in_30"}, -- NB relies on 2 clusters
                {irq=6,  dst = "platform.hexagon_cluster_0.l2vic.irq_in_30"},
            }
        };

    gpex = { pio_iface = { address = 0x003eff0000, size = 0x0000010000 };
        mmio_iface = { address = 0x0060000000, size = 0x002B500000 };
        ecam_iface = { address = 0x4010000000, size = 0x0010000000 };
        mmio_iface_high = { address = 0x8000000000, size = 0x8000000000 },
        irq_0 = 0, irq_1 = 0, irq_2 = 0, irq_3 = 0
    };

    -- qtb = { control_socket = {address=0x15180000, size=0x80000}};
    -- The QTB model is not active in the system, left here for debug purposes.

    smmu = { socket = {address=0x15000000, size=0x100000};
             num_tbu=2;  -- for now, this needs to match the expected number of TBU's
             num_pages=128;
             num_cb=128;
             irq_context = 103;
             irq_global = 65;
           };

    -- Turing_NSP_0, ARID 14, AC_VM_CDSP_Q6_ELF:
    tbu_0 = { topology_id=0x21C0,
              upstream_socket = { address=NSP0_AHB_HIGH,
                                  size=0xF00000000-NSP0_AHB_HIGH,
                                  relative_addresses=false
             }
            };
    -- Turing_NSP_1, ARID 14, AC_VM_CDSP_Q6_ELF:
    tbu_1 = { topology_id=0x29C0,
             upstream_socket = { address=NSP1_AHB_HIGH,
                                 size=0xF00000000-NSP1_AHB_HIGH,
                                 relative_addresses=false
             }
            };

    memorydumper = { target_socket={address=0x1B300040, size=0x10}},

    fallback_memory = { target_socket={address=0x0, size=0x40000000},
                        dmi_allow=false, verbose=true,
                        load={csv_file=MAKENA_REGS_CSV,
                        offset=0, addr_str="Address",
                        value_str="Reset Value", byte_swap=true}
                      };
    load={
        {data={0x60190200}, address=TCSR_SOC_HW_VERSION_ADDR};
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
        {data={0x4},
         address=HWKM_MASTER_CFG_KM_RNG_EE10_PRNG_DATA_OUT};
        {data={0x01},
         address=HWKM_MASTER_CFG_KM_RNG_EE10_PRNG_STATUS};


--      {data={0x00020400}, address=0x10E0000};
--      {data={0x60000003}, address=0x10E000C};
--      {data={0x0}, address=TCSR_SOC_EMULATION_TYPE_ADDR}; -- 0=silicon
--      {data={0x1}, address=TCSR_SOC_EMULATION_TYPE_ADDR}; -- 1=RUMI
    };
    -- uart_backend_port=4001;
    -- qup_uart_backend_port=4001;
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


-- read in any local configuration (Before convenience switches)
local IMAGE_DIR;
if os.getenv("QQVP_IMAGE_DIR") == nil then
    IMAGE_DIR = "./"
else
    IMAGE_DIR = os.getenv("QQVP_IMAGE_DIR").."/"
end

if (file_exists(IMAGE_DIR.."conf.lua"))
then
    print ("Running local "..IMAGE_DIR.."conf.lua");
    dofile(IMAGE_DIR.."conf.lua");
else
    print ("A local conf.lua file is required in the directory containing your images. That defaults to the current directory or you can set it using QQVP_IMAGE_DIR\n");
    os.exit(1);
end


-- convenience switches
if (platform.with_gpu) then
    tableMerge(platform, {
        gpex=     { pio_iface             = {address=0x003eff0000, size=0x0000010000};
        mmio_iface            = {address=0x0060000000, size=0x002B500000};
        ecam_iface            = {address=0x4010000000, size=0x0001000000};
        mmio_iface_high       = {address=0x8000000000, size=0x8000000000},
    irq_0=0, irq_1=0, irq_2=0, irq_3=0};
    });
end


print ("Lua config Finished.");
