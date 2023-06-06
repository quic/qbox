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

dofile(top().."../utils.lua");
print ("Lua config running. . . ");

-- these values are helpful for local configuration, so make them global
 INITIAL_DDR_SPACE_14GB = 0x80000000
 OFFSET_MIFS_DDR_SPACE  = 0x60000000
 OFFSET_SMEM_DDR_SPACE  = 0x10900000
 DDR_SPACE_SIZE = 16*1024*1024*1024


local HEX_DIGITS='0123456789ABCDEF'

local MAKENA_REGS_CSV = valid_file(top().."8540_Registers.csv")
local QDSP6_CFG   = valid_file(top().."../qdsp6.lua")

local NSP0_BASE     = 0x24000000 -- TURING_SS_0TURING
local NSP1_BASE     = 0x28000000 -- TURING_SS_1TURING
   -- ^^ This val is also described as 'TCM_BASE_ADDRESS' in
   -- Sys arch spec "TCM region memory map"
local TURING_SS_0TURING_QDSP6V68SS_CSR= 0x26380000
local TURING_SS_1TURING_QDSP6V68SS_CSR= 0x2A380000
local LPASS_QDSP6V68SS_CSR= 0x03080000
-- The QDSP6v67SS private registers include CSR, L2VIC, QTMR registers.
-- The address base is determined by parameter QDSP6SS_PRIV_BASEADDR

-- ipcat says LPASS subsystem starts at 0x2c00000, with a 4MiB "unused"
-- region. However, our calculations from get_dsp() only match the expected
-- registers if we consider LPASS' base to start at 0x200000 further, right in
-- the middle of the "unused" region.
local LPASS_BASE = 0x2c00000 + 0x200000

-- These values were pulled out from the silicom by SiVal team
local NSP0_AHB_LOW = 0x2600000 -- TURINGNSP_0_TURING_QDSP6SS_STRAP_AHBLOWER_STATUS
local NSP0_AHB_HIGH = 0x2700000 -- TURINGNSP_0_TURING_QDSP6SS_STRAP_AHBUPPER_STATUS
local NSP0_AHB_SIZE = NSP0_AHB_HIGH - NSP0_AHB_LOW
local NSP1_AHB_LOW = 0x2A00000 -- TURINGNSP_1_TURING_QDSP6SS_STRAP_AHBLOWER_STATUS
local NSP1_AHB_HIGH = 0x2B00000 -- TURINGNSP_1_TURING_QDSP6SS_STRAP_AHBUPPER_STATUS
local NSP1_AHB_SIZE = NSP1_AHB_HIGH - NSP1_AHB_LOW
local ADSP_AHB_LOW = 0x300000 -- LPASS_QDSP6SS_STRAP_AHBLOWER_STATUS
local ADSP_AHB_HIGH = 0x390000 -- LPASS_QDSP6SS_STRAP_AHBUPPER_STATUS
local ADSP_AHB_SIZE = ADSP_AHB_HIGH - ADSP_AHB_LOW
-- Unused:
-- local TURINGGDSP_0_TURING_QDSP6SS_STRAP_AHBUPPER_STATUS=0x2100000
-- local TURINGGDSP_0_TURING_QDSP6SS_STRAP_AHBLOWER_STATUS=0x2080000
-- local TURINGGDSP_1_TURING_QDSP6SS_STRAP_AHBUPPER_STATUS=0x2200000
-- local TURINGGDSP_1_TURING_QDSP6SS_STRAP_AHBLOWER_STATUS=0x2180000

local IPC_ROUTER_TOP = 0x00400000


local APSS_GIC600_GICD_APSS = 0x17A00000
local OFFSET_APSS_ALIAS0_GICR_CTLR = 0x60000

local UART0 = 0x10000000

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
local HWKM_MASTER_CFG_KW_RNG_EEx_base=0x010D0000;

local LPASS_QDSP6SS_RSCC_RSC_PARAM_RSC_CONFIG_DRVd = 0x30B0008;
local LPASS_RSCC_RSC_PARAM_RSC_CONFIG_DRVd = 0x3480008;

dofile(QDSP6_CFG);

-- LeMans: NSP0 configuration --
local SA8775P_nsp0_config_table= get_SA8775P_nsp0_config_table();
local nsp0ss = get_dsp(
        "v73",
        0x24000000, -- TURING_SS_0TURING
        0x26300000, -- TURING_SS_0TURING_QDSP6V68SS
        SA8775P_nsp0_config_table,
        0x9B800000, -- entry point address from bootimage_lemans.cdsp0.prodQ.pbn
        NSP0_AHB_SIZE,
        6 -- threads
        );
assert((SA8775P_nsp0_config_table[3] << 16) == TURING_SS_0TURING_QDSP6V68SS_CSR)
assert((SA8775P_nsp0_config_table[3] << 16) == TURING_SS_0TURING_QDSP6V68SS_CSR)
local NSP0_VTCM_BASE_ADDR = (SA8775P_nsp0_config_table[15] << 16)
local NSP0_VTCM_SIZE_BYTES = (SA8775P_nsp0_config_table[16] * 1024)


-- NSP1 configuration TODO : need ramdump of config_table for NSP1. --
local SA8775P_nsp1_config_table= get_SA8775P_nsp1_config_table();
local nsp1ss = get_dsp(
        "v73",
        0x28000000, -- TURING_SS_1TURING
        0x2A300000, -- TURING_SS_1TURING_QDSP6V68SS
        SA8775P_nsp1_config_table,
        0x9D700000, -- entry point address from bootimage_lemans.cdsp1.prodQ.pbn
        NSP1_AHB_SIZE,
        6 -- threads
        );
assert((SA8775P_nsp1_config_table[11] << 16) == nsp1ss.l2vic.fastmem.address)
assert((SA8775P_nsp1_config_table[3] << 16) == TURING_SS_1TURING_QDSP6V68SS_CSR)
local NSP1_VTCM_BASE_ADDR = (SA8775P_nsp1_config_table[15] << 16)
local NSP1_VTCM_SIZE_BYTES = (SA8775P_nsp1_config_table[16] * 1024)

-- ADSP configuration
local SA8775P_adsp_config_table= get_SA8775P_adsp_config_table();
local adsp = get_dsp(
        "v66",
        LPASS_BASE,
        0x03000000, -- LPASS_QDSP6V68SS_PUB
        SA8775P_adsp_config_table,
        0x95C00000, -- entry point address from bootimage_lemans.adsp.prodQ.pbn
        ADSP_AHB_SIZE,
        2 -- threads
        );
assert((SA8775P_adsp_config_table[11] << 16) == adsp.l2vic.fastmem.address)
assert((SA8775P_adsp_config_table[3] << 16) == LPASS_QDSP6V68SS_CSR)

local IS_SHARED_MEM = false
if os.getenv("QQVP_PLUGIN_DIR") ~= nil then
    IS_SHARED_MEM = true
end


platform = {
    arm_num_cpus = 8;
    num_redists = 1;
    with_gpu = false;

    hexagon_num_clusters = 3;
    hexagon_cluster_0 = nsp0ss;
    hexagon_cluster_1 = nsp1ss;
    hexagon_cluster_2 = adsp;
    quantum_ns = 10000000;

    ArmQemuInstance = { tcg_mode="MULTI", sync_policy = "multithread-unconstrained"};

    ram_0=  {  target_socket = {address=INITIAL_DDR_SPACE_14GB, size=DDR_SPACE_SIZE/2}, shared_memory=IS_SHARED_MEM};
    ram_1=  {  target_socket = {address=INITIAL_DDR_SPACE_14GB+(DDR_SPACE_SIZE/2), size=DDR_SPACE_SIZE/2}, shared_memory=IS_SHARED_MEM};
    hexagon_ram_0={target_socket={address=NSP0_VTCM_BASE_ADDR, size=NSP0_VTCM_SIZE_BYTES}};
    hexagon_ram_1={target_socket={address=NSP1_VTCM_BASE_ADDR, size=NSP1_VTCM_SIZE_BYTES}};
    gic=  {  dist_iface    = {address=APSS_GIC600_GICD_APSS, size= OFFSET_APSS_ALIAS0_GICR_CTLR};
             redist_iface_0= {address=APSS_GIC600_GICD_APSS+OFFSET_APSS_ALIAS0_GICR_CTLR, size=0x1C0000}};
    virtionet0= { mem    =   {address=0x1c120000, size=0x10000}, irq=18, netdev_str="type=user,hostfwd=tcp::2222-:22,hostfwd=tcp::2221-:21,hostfwd=tcp::56283-:56283,hostfwd=tcp::55534-:65534,hostfwd=tcp::55535-:65535"};
    rtl8139 = { netdev_str = "type=user"}; -- MAC address can be overriden using mac="<mac address>", a Qemu generated MAC address is used by default.
    --    virtioblk_0= { mem    =   {address=0x1c0d0000, size=0x2000}, irq=9, blkdev_str="file="..SYSTEM_QDRIVE..",format=raw,if=none"};

    qtimer_0=   { mem     =   {address=0x17c20000, size=0x1000};
                  mem_view=   {address=0x17c21000, size=0x1000*2*7}; -- 0x1000*nr_frames*nr_views
                  irq0=40;irq1=41;irq2=42;irq3=43;irq4=44;irq5=45;irq6=46;
                  nr_frames=7;nr_views=2;cnttid=0x1111515};
    uart = {  simple_target_socket_0 = {address= UART0, size=0x1000},
        irq=379,
        stdio=true,
        input=false,
        port=nil,
    };

    ipcc= {  socket        = {address=IPC_ROUTER_TOP, size=0xfc000},
             irqs = {
                {irq=8, dst = "platform.gic.spi_in_229"},
                {irq=18, dst = "platform.hexagon_cluster_1.l2vic.irq_in_30"}, -- NB relies on 2 clusters
                {irq=6,  dst = "platform.hexagon_cluster_0.l2vic.irq_in_30"},
                {irq=3,  dst = "platform.hexagon_cluster_2.l2vic.irq_in_153"},
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

    smmu_0 = { socket = {address=0x15000000, size=0x100000};
             num_tbu=3;  -- for now, this needs to match the expected number of TBU's
             num_pages=128;
             num_cb=128;
             num_smr=224;
             irq_context = 103;
             irq_global = 65;
           };
    smmu_1 = { socket = {address=0x15200000, size=0x100000};
             num_tbu=0;
             num_pages=64;
             num_cb=64;
             num_smr=64;
             irq_context = 925;
             irq_global = 920;
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

    tbu_2 = { topology_id=0x3000,
             upstream_socket = { address=ADSP_AHB_HIGH,
                                 size=0xF00000000-ADSP_AHB_HIGH,
                                 relative_addresses=false
             }
            };

    memorydumper = { target_socket={address=0x1B300040, size=0x10}},

    fallback_memory = { target_socket={address=0x0, size=0x40000000},
                        dmi_allow=false, verbose=true,
                        log_level=3,
                        load={csv_file=MAKENA_REGS_CSV,
                        offset=0, addr_str="Address",
                        value_str="Reset Value", byte_swap=true}
                      };
    load={
        {data={0x60190200}, address=TCSR_SOC_HW_VERSION_ADDR};
        {data={0x04}, address=TCSR_SOC_EMULATION_TYPE_ADDR};
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
        {data={0x01300214},
         address=LPASS_QDSP6SS_RSCC_RSC_PARAM_RSC_CONFIG_DRVd};
        {data={0x01180214},
         address=LPASS_RSCC_RSC_PARAM_RSC_CONFIG_DRVd};
        -- RPMH_PDC_AUDIO_PDC_PARAM_RESOURCE_DRVd
        {data={0x000A5381}, address=0xB251004};

--      {data={0x00020400}, address=0x10E0000};
--      {data={0x60000003}, address=0x10E000C};
    };
};


-- PRNG values for HWKM_MASTER_CFG_KM_RNG_EE{2 to 15}
local PRNG_table = {}
for id = 3, #HEX_DIGITS do
  local data_addr = HWKM_MASTER_CFG_KW_RNG_EEx_base +
                    tonumber("0x" .. HEX_DIGITS:sub(id,id) .. "000")
  local status_addr = data_addr + 0x4
  table.insert(PRNG_table, {data={0x4}, address=data_addr})
  table.insert(PRNG_table, {data={0x01}, address=status_addr})
end
tableJoin(platform["load"], PRNG_table)

if (platform.arm_num_cpus > 0) then
    local QUP_PITCH = 0x4000;
    local QUP_SIZE = 0x2000;
    local QUP_BANKS = {
        -- QUPV3_2 / u_qupv3_wrapper_2 / qupv3_se_irq[*]:
        { addr = 0x880000, primary_idx = nil,
          qgic_spi_irqs = { 373, 583, 584, 585, 586, 587, 833, },
        },
        -- QUPV3_0 / u_qupv3_wrapper_0 / qupv3_se_irq[*]:
        { addr = 0x980000, primary_idx = nil,
          qgic_spi_irqs = { 550, 551, 529, 530, 531, 535, 536, },
        },
        -- QUPV3_1 / u_qupv3_wrapper_1 / qupv3_se_irq[*]:
        { addr = 0xa80000, primary_idx = 3,
          qgic_spi_irqs = { 353, 354, 355, 356, 357, 358, 835, },
        },
    };

    local qup_index = 0;
    for _, bank in next, QUP_BANKS do
        -- assert(bank.count == #bank.qgic_spi_irqs)
        for i, spi_irq in next, bank.qgic_spi_irqs do
            local i_0 = i - 1;
            local qup = {
                simple_target_socket_0 = {address=bank.addr + (i_0*QUP_PITCH),
                    size=QUP_SIZE},
                irq=spi_irq,
                stdio=true,
                input=(i_0 == bank.primary_idx),
                port=nil,
            };
            platform["uart_qup_"..tostring(qup_index)] = qup;
            qup_index = qup_index + 1;
        end
    end


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

local PLUGIN_DIR;
if os.getenv("QQVP_PLUGIN_DIR") == nil then
    PLUGIN_DIR = "./"
else
    PLUGIN_DIR = os.getenv("QQVP_PLUGIN_DIR").."/"
end

if (file_exists(PLUGIN_DIR.."conf.lua"))
then
    print ("Running local "..PLUGIN_DIR.."conf.lua");
    dofile(PLUGIN_DIR.."conf.lua");
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
