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

INITIAL_DDR_SPACE_14GB = 0x80000000

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
local NSP0_AHB_LOW = 0x26000000 -- TURINGNSP_0_TURING_QDSP6SS_STRAP_AHBLOWER_STATUS
local NSP0_AHB_HIGH = 0x27000000 -- TURINGNSP_0_TURING_QDSP6SS_STRAP_AHBUPPER_STATUS
local NSP0_AHB_SIZE = NSP0_AHB_HIGH - NSP0_AHB_LOW
local NSP1_AHB_LOW = 0x2A000000 -- TURINGNSP_1_TURING_QDSP6SS_STRAP_AHBLOWER_STATUS
local NSP1_AHB_HIGH = 0x2B000000 -- TURINGNSP_1_TURING_QDSP6SS_STRAP_AHBUPPER_STATUS
local NSP1_AHB_SIZE = NSP1_AHB_HIGH - NSP1_AHB_LOW
local ADSP_AHB_LOW = 0x3000000 -- LPASS_QDSP6SS_STRAP_AHBLOWER_STATUS
local ADSP_AHB_HIGH =0x3900000 -- LPASS_QDSP6SS_STRAP_AHBUPPER_STATUS
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

local IS_SHARED_MEM = false
if os.getenv("QQVP_PLUGIN_DIR") ~= nil then
    IS_SHARED_MEM = true
end

-- LeMans: NSP0 configuration --
local SA8775P_nsp0_config_table= get_SA8775P_nsp0_config_table();
local NSP0_VTCM_BASE_ADDR = (SA8775P_nsp0_config_table[15] << 16)
local NSP0_VTCM_SIZE_BYTES = (SA8775P_nsp0_config_table[16] * 1024)
local nsp0ss = get_dsp(
        "v73",
        NSP0_BASE, -- TURING_SS_0TURING
        0x26300000, -- TURING_SS_0TURING_QDSP6V68SS
        SA8775P_nsp0_config_table,
        0x9B800000, -- entry point address from bootimage_lemans.cdsp0.prodQ.pbn
        NSP0_AHB_SIZE,
        6, -- threads
        "&qemu_hex_inst_0",
        NSP0_AHB_HIGH - NSP0_BASE, -- region total size
        1, -- num_of_vtcm
        NSP0_VTCM_BASE_ADDR, -- vtcm_base
        NSP0_VTCM_SIZE_BYTES, -- vtcm_size
        IS_SHARED_MEM -- is_vtcm_shared
        );
assert((SA8775P_nsp0_config_table[3] << 16) == TURING_SS_0TURING_QDSP6V68SS_CSR)
assert((SA8775P_nsp0_config_table[3] << 16) == TURING_SS_0TURING_QDSP6V68SS_CSR)


-- NSP1 configuration TODO : need ramdump of config_table for NSP1. --
local SA8775P_nsp1_config_table= get_SA8775P_nsp1_config_table();
local NSP1_VTCM_BASE_ADDR = (SA8775P_nsp1_config_table[15] << 16)
local NSP1_VTCM_SIZE_BYTES = (SA8775P_nsp1_config_table[16] * 1024)
local nsp1ss = get_dsp(
        "v73",
        NSP1_BASE, -- TURING_SS_1TURING
        0x2A300000, -- TURING_SS_1TURING_QDSP6V68SS
        SA8775P_nsp1_config_table,
        0x9D700000, -- entry point address from bootimage_lemans.cdsp1.prodQ.pbn
        NSP1_AHB_SIZE,
        6, -- threads
        "&qemu_hex_inst_1",
        NSP1_AHB_HIGH - NSP1_BASE, -- region total size
        1, -- num_of_vtcm
        NSP1_VTCM_BASE_ADDR, -- vtcm_base
        NSP1_VTCM_SIZE_BYTES, -- vtcm_size
        IS_SHARED_MEM -- is_vtcm_shared
        );
assert((SA8775P_nsp1_config_table[11] << 16) == nsp1ss.l2vic.fastmem.address)
assert((SA8775P_nsp1_config_table[3] << 16) == TURING_SS_1TURING_QDSP6V68SS_CSR)

-- ADSP configuration
local SA8775P_adsp_config_table= get_SA8775P_adsp_config_table();
local adsp = get_dsp(
        "v66",
        LPASS_BASE,
        0x03000000, -- LPASS_QDSP6V68SS_PUB
        SA8775P_adsp_config_table,
        0x95C00000, -- entry point address from bootimage_lemans.adsp.prodQ.pbn
        ADSP_AHB_SIZE,
        2, -- threads
        "&qemu_hex_inst_2",
        (0x03000000 - LPASS_BASE) + 0x100000, -- region total size
        0, -- num_of_vtcm
        0, -- vtcm_base
        0, -- vtcm_size
        IS_SHARED_MEM -- is_vtcm_shared
        );
assert((SA8775P_adsp_config_table[11] << 16) == adsp.l2vic.fastmem.address)
assert((SA8775P_adsp_config_table[3] << 16) == LPASS_QDSP6V68SS_CSR)

local IS_SHARED_MEM = false
if os.getenv("QQVP_PLUGIN_DIR") ~= nil then
    IS_SHARED_MEM = true
end

-- these values are helpful for local configuration, so make them global
 OFFSET_MIFS_DDR_SPACE  = 0x60000000
 OFFSET_SMEM_DDR_SPACE  = 0x10900000
-- Provided for backwards compatibility with local configs that use this var:
 INITIAL_DDR_SPACE_14GB = 0x80000000

if ACCEL == "hvf" then
    print("Loading arm64 bootloader")
    _KERNEL64_LOAD_ADDR = INITIAL_DDR_SPACE_14GB + OFFSET_MIFS_DDR_SPACE
    local ARM64_BOOTLOADER = valid_file(top() .. "../arm64_bootloader.lua")
    dofile(ARM64_BOOTLOADER)
end

if ACCEL == nil then
    ACCEL = "tcg"
end
print("Virtual acceleration: " .. ACCEL)

local ARCH_TIMER_VIRT_IRQ = 16 + 11
local ARCH_TIMER_S_EL1_IRQ = 16 + 13
local ARCH_TIMER_NS_EL1_IRQ = 16 + 14
local ARCH_TIMER_NS_EL2_IRQ = 16 + 10

ARM_NUM_CPUS = 8;
local NUM_REDISTS = 1;
-- local HEXAGON_NUM_CLUSTERS = 3;

zipfile = valid_file(top().."../top_SA8775.zip");

platform = {
    with_gpu = false;

    moduletype="Container";

    hex_plugin = {
        moduletype = "Container", -- can be replaced by 'RemotePass'
        -- exec_path = "./build/tests/rplugins/vp-hex-remote";
        -- remote_argv = {"--param=log_level=2"},
        tlm_initiator_ports_num = 6,
        tlm_target_ports_num = 3,
        target_signals_num = 3,
        initiator_signals_num = 0,
        -- https://ipcatalog.qualcomm.com/memmap/chip/434/map/1356/version/11766/block/24917388
        target_socket_0 = {address=NSP0_BASE , size= NSP0_AHB_HIGH - NSP0_BASE, relative_addresses=false, bind = "&router.initiator_socket"}; --nsp0ss
        target_socket_1 = {address=NSP1_BASE , size= NSP1_AHB_HIGH - NSP1_BASE, relative_addresses=false, bind = "&router.initiator_socket"}; --nsp1ss
        target_socket_2 = {address=LPASS_BASE , size= (0x03000000 - LPASS_BASE) + 0x100000, relative_addresses=false, bind = "&router.initiator_socket"}; --adsp
        initiator_socket_0 = {bind = "&router.target_socket"}, --nsp0ss
        initiator_socket_2 = {bind = "&router.target_socket"}, --nsp1ss
        initiator_socket_4 = {bind = "&router.target_socket"}, --adsp

        plugin_pass = {
            moduletype = "LocalPass", -- -- can be replaced by 'RemotePass'
            tlm_initiator_ports_num = 3,
            tlm_target_ports_num = 6,
            target_signals_num = 0,
            initiator_signals_num = 3,

            initiator_signal_socket_0 = {bind = "&hexagon_cluster_0.l2vic.irq_in_30"}, --nsp0ss
            initiator_signal_socket_1 = {bind = "&hexagon_cluster_1.l2vic.irq_in_30"}, --nsp1ss
            initiator_signal_socket_2 = {bind = "&hexagon_cluster_2.l2vic.irq_in_153"}, --adsp
            --nsp0ss
            target_socket_0 = {address=0x0, size=NSP0_BASE, relative_addresses=false, bind = "&hexagon_cluster_0.router.initiator_socket"},
            target_socket_1 = {address=NSP0_AHB_HIGH, size= 0xF00000000-NSP0_AHB_HIGH, relative_addresses=false, bind = "&hexagon_cluster_0.router.initiator_socket"},
            --nsp1ss
            target_socket_2 = {address=0x0, size=NSP1_BASE, relative_addresses=false, bind = "&hexagon_cluster_1.router.initiator_socket"},
            target_socket_3 = {address=NSP1_AHB_HIGH, size= 0xF00000000-NSP1_AHB_HIGH, relative_addresses=false, bind = "&hexagon_cluster_1.router.initiator_socket"},
            --adsp
            target_socket_4 = {address=0x0, size=LPASS_BASE, relative_addresses=false, bind = "&hexagon_cluster_2.router.initiator_socket"},
            target_socket_5 = {address=ADSP_AHB_HIGH, size= 0xF00000000-ADSP_AHB_HIGH, relative_addresses=false, bind = "&hexagon_cluster_2.router.initiator_socket"},

            initiator_socket_0 = {bind = "&hexagon_cluster_0.router.target_socket"}, --nsp0ss
            initiator_socket_1 = {bind = "&hexagon_cluster_1.router.target_socket"}, --nsp1ss
            initiator_socket_2 = {bind = "&hexagon_cluster_2.router.target_socket"}, --adsp
        },

        qemu_inst_mgr_h = {
            moduletype = "QemuInstanceManager";
        },

        qemu_hex_inst_0= {
            moduletype="QemuInstance";
            args = {"&qemu_inst_mgr_h", "HEXAGON"};
            tcg_mode="SINGLE",
            sync_policy = "multithread-unconstrained"
        },

        qemu_hex_inst_1= {
            moduletype="QemuInstance";
            args = {"&qemu_inst_mgr_h", "HEXAGON"};
            tcg_mode="SINGLE",
            sync_policy = "multithread-unconstrained"
        },

        qemu_hex_inst_2= {
            moduletype="QemuInstance";
            args = {"&qemu_inst_mgr_h", "HEXAGON"};
            tcg_mode="SINGLE",
            sync_policy = "multithread-unconstrained"
        },


        hexagon_cluster_0 = nsp0ss;
        hexagon_cluster_1 = nsp1ss;
        hexagon_cluster_2 = adsp;
    },

    quantum_ns = 10000000;

    router = {
        moduletype="Router";
        log_level=0;
        -- target_socket = {bind = "&tbu_2.downstream_socket"};
    },

   DDR_space = {moduletype="Memory";
               target_socket = {bind = "&router.initiator_socket";},
               shared_memory=IS_SHARED_MEM};

    -- ram_0=  {
    --             moduletype="Memory";
    --             target_socket = {"&platform.DDR_space", bind= "&router.initiator_socket"},
    --             shared_memory=IS_SHARED_MEM};

   DDR_space_1 = {moduletype="Memory";
               target_socket = {bind = "&router.initiator_socket";},
               shared_memory=IS_SHARED_MEM};

    -- ram_1=  {
    --             moduletype="Memory";
    --             target_socket= {"&platform.DDR_space_1", bind = "&router.initiator_socket"},
    --             shared_memory=IS_SHARED_MEM};

    qemu_inst_mgr = {
        moduletype = "QemuInstanceManager";
    },

    qemu_inst= {
        moduletype="QemuInstance";
        args = {"&platform.qemu_inst_mgr", "AARCH64"};
        tcg_mode="MULTI",
        sync_policy = "multithread-unconstrained"
    },

    gic_0 =  {
                moduletype = "QemuArmGicv3",
                args = {"&platform.qemu_inst"},
                dist_iface    = {address=APSS_GIC600_GICD_APSS, size= OFFSET_APSS_ALIAS0_GICR_CTLR, bind = "&router.initiator_socket"};
                redist_iface_0= {address=APSS_GIC600_GICD_APSS+OFFSET_APSS_ALIAS0_GICR_CTLR, size=0x1C0000, bind = "&router.initiator_socket"};
                num_cpus = ARM_NUM_CPUS,
                redist_region = {ARM_NUM_CPUS / NUM_REDISTS};
                num_spi=960
            };
    virtionet0_0= {
                    moduletype = "QemuVirtioMMIONet",
                    args = {"&platform.qemu_inst"};
                    mem    =   {address=0x1c120000, size=0x10000, bind = "&router.initiator_socket"},
                    irq_out = {bind = "&gic_0.spi_in_18"},
                    netdev_str="type=user,hostfwd=tcp::2222-:22,hostfwd=tcp::2221-:21,hostfwd=tcp::56283-:56283,hostfwd=tcp::55534-:65534,hostfwd=tcp::55535-:65535"};

    rtl8139_0= {
        moduletype = "QemuRtl8139Pci",
        args = {"&platform.qemu_inst", "&platform.gpex_0"},
        netdev_str = "type=user",
    };

    mpm_0 = {
        moduletype="mpm_control",
        socket = {address=0x0C210000, size=0x1000, bind="&router.initiator_socket"},
    };

    qtimer_0=   {
                    moduletype = "QemuHexagonQtimer",
                    args = {"&platform.qemu_inst"};
                    mem     =   {address=0x17c20000, size=0x1000, bind = "&router.initiator_socket"};
                    mem_view=   {address=0x17c21000, size=0x1000*2*7, bind = "&router.initiator_socket"}; -- 0x1000*nr_frames*nr_views
                    irq_0 = {bind = "&gic_0.spi_in_40"},
                    irq_1 = {bind = "&gic_0.spi_in_41"},
                    irq_2 = {bind = "&gic_0.spi_in_42"},
                    irq_3 = {bind = "&gic_0.spi_in_43"},
                    irq_4 = {bind = "&gic_0.spi_in_44"},
                    irq_5 = {bind = "&gic_0.spi_in_45"},
                    irq_6 = {bind = "&gic_0.spi_in_46"},
                    nr_frames=7;nr_views=2;cnttid=0x1111515};

    -- charbackend_stdio_0 = {
    --     moduletype = "CharBackendStdio";
    --     args = {false};
    -- };

    -- pl011 = {
    --     moduletype = "Pl011",
    --     args = {"&platform.charbackend_stdio_0"};
    --     target_socket = {address= UART0,
    --                               size=0x1000,
    --                               bind = "&router.initiator_socket"},
    --     irq = {bind = "&gic_0.spi_in_379"},
    -- };

    ipcc_0= {
            moduletype="IPCC",
            target_socket = {address=IPC_ROUTER_TOP, size=0xfc000, bind = "&router.initiator_socket"},
            irq_8 = {bind = "&gic_0.spi_in_229"},
            irq_18 = {bind = "&hex_plugin.target_signal_socket_1"},
            irq_6 = {bind = "&hex_plugin.target_signal_socket_0"},
            irq_3 = {bind = "&hex_plugin.target_signal_socket_2"},
        };
    mpm = { socket = {address=0x0C210000, size=0x1000}};

    gpex_0 = {
        moduletype = "QemuGPEX";
        args = {"&platform.qemu_inst"};
        bus_master = {bind = "&router.target_socket"};
        pio_iface = { address = 0x003eff0000, size = 0x0000010000, bind= "&router.initiator_socket"};
        mmio_iface = { address = 0x0060000000, size = 0x002B500000, bind= "&router.initiator_socket" };
        ecam_iface = { address = 0x4010000000, size = 0x0010000000, bind= "&router.initiator_socket" };
        mmio_iface_high = { address = 0x8000000000, size = 0x8000000000, bind= "&router.initiator_socket" },
        irq_out_0 = {bind = "&gic_0.spi_in_0"};
        irq_out_1 = {bind = "&gic_0.spi_in_0"};
        irq_out_2 = {bind = "&gic_0.spi_in_0"};
        irq_out_3 = {bind = "&gic_0.spi_in_0"};
        irq_num_0 = 0;
        irq_num_1 = 0;
        irq_num_2 = 0;
        irq_num_3 = 0;
    };

    usb_0 = {
        moduletype = "QemuXhci",
        args = {"&platform.qemu_inst", "&platform.gpex_0"},
    };

    kbd_0 = {
        moduletype = "QemuKbd",
        args = {"&platform.qemu_inst", "&platform.usb_0"},
    };

    tablet_0 = {
        moduletype = "QemuTablet",
        args = {"&platform.qemu_inst", "&platform.usb_0"},
    };

    -- qtb = { control_socket = {address=0x15180000, size=0x80000}};
    -- The QTB model is not active in the system, left here for debug purposes.

    -- Turing_NSP_0, ARID 14, AC_VM_CDSP_Q6_ELF:

    tbu_0 = {
        moduletype = "smmu500_tbu",
        args = {"&platform.smmu_0"},
        topology_id=0x21C0,
        upstream_socket = {
                            -- address=NSP0_AHB_HIGH,
                            -- size=0xF00000000-NSP0_AHB_HIGH,
                            -- relative_addresses=false,
                            bind = "&hex_plugin.initiator_socket_1"
       },
       downstream_socket = {bind = "&router.target_socket"};
      };

    tbu_1 = {
       moduletype = "smmu500_tbu",
       args = {"&platform.smmu_0"},
       topology_id=0x29C0,
       upstream_socket = {
                        --    address=NSP1_AHB_HIGH,
                        --    size=0xF00000000-NSP1_AHB_HIGH,
                        --    relative_addresses=false,
                           bind = "&hex_plugin.initiator_socket_3"
       },
       downstream_socket = {bind = "&router.target_socket"};
      };

    tbu_2 = {
        moduletype = "smmu500_tbu",
        args = {"&platform.smmu_0"},
        topology_id=0x3000,
        upstream_socket = {
                            -- address=ADSP_AHB_HIGH,
                            -- size=0xF00000000-ADSP_AHB_HIGH,
                            -- relative_addresses=false,
                            bind = "&hex_plugin.initiator_socket_5"
             },
             downstream_socket = {bind = "&router.target_socket"};
       };

    -- remote_0 = {
    --     moduletype = "PassRPC";
    --     args = {"./vp-hex-remote"};
    --     target_socket_0 = {address= 0xAC00000,
    --                         size= 0x200000,
    --                         relative_addresses=false;
    --                         bind = "&router.initiator_socket"};
    --     irq_0 = 465,
    --     irq_1 = 469,
    -- };

    memorydumper_0 = {
        moduletype = "MemoryDumper",
        initiator_socket = {bind = "&router.target_socket"};
        target_socket={address=0x1B300040,
                        size=0x10,
                        bind = "&router.initiator_socket"}
            },

    fallback_0={
        moduletype="json_fallback_module";
        target_socket={dynamic=true, bind="&router.initiator_socket"},
        log_level=0,
        zipfile=zipfile
    };

--[[ Temporary, substitute this for fallback_0 stanza above when booting Gunyah

      fallback_0 = {
          moduletype="Memory",
          target_socket={address=0x0, size=0x40000000, dynamic=true, bind="&router.initiator_socket"},
          dmi_allow=false, verbose=true,
          log_level=0,
          load={csv_file=MAKENA_REGS_CSV,
          offset=0, addr_str="Address",
          value_str="Reset Value", byte_swap=true}
     };
--]]


    global_peripheral_initiator_arm_0 = {
            moduletype = "GlobalPeripheralInitiator",
            args = {"&platform.qemu_inst", "&platform.cpu_0"},
            global_initiator = {bind = "&router.target_socket"},
    };

    load={
        moduletype = "Loader",
        initiator_socket = {bind = "&router.target_socket"};
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

        -- GCC_QUPV3_WRAP0_S3_CFG_RCGR
        {data={0x2601}, address=0x1234f4};
        -- GCC_QUPV3_WRAP0_S3_M
        {data={0x180}, address=0x1234f8};
        -- GCC_QUPV3_WRAP0_S3_N
        {data={0xc476}, address=0x1234fc};
        -- GCC_QUPV3_WRAP0_S3_D
        {data={0xc2f6}, address=0x123500};

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

local irq_context = 103
local irq_global = 65
local num_cb = 128

local smmu_index = 0;

local smmu = {
    moduletype = "smmu500",
    dma = {bind = "&router.target_socket"},
    target_socket = {address=0x15000000, size=0x100000, bind= "&router.initiator_socket"},
    num_tbu = 2,
    num_pages = 128,
    num_cb = 128;
    num_smr = 224,
    irq_global = {bind = "&gic_0.spi_in_"..irq_global},
}
for i = 0, num_cb - 1 do
    smmu["irq_context_" .. i] = {bind = "&gic_0.spi_in_" .. irq_context + i}
end
platform["smmu_"..tostring(smmu_index)] = smmu;


irq_context = 896
irq_global = 920
num_cb = 64
smmu_index = 1;

smmu = {
    moduletype = "smmu500",
    dma = {bind = "&router.target_socket"},
    target_socket = {address=0x15200000, size=0x100000, bind= "&router.initiator_socket"},
    num_tbu = 0,
    num_pages = 64,
    num_cb = 64;
    num_smr = 64,
    irq_global = {bind = "&gic_0.spi_in_"..irq_global},
}

for i = 0, num_cb - 1 do
    smmu["irq_context_" .. i] = {bind = "&gic_0.spi_in_" .. irq_context + i}
end
platform["smmu_"..tostring(smmu_index)] = smmu;

if (ARM_NUM_CPUS > 0) then
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

    qup_index = 0;
    for _, bank in next, QUP_BANKS do
        -- assert(bank.count == #bank.qgic_spi_irqs)
        for i, spi_irq in next, bank.qgic_spi_irqs do
            local i_0 = i - 1;
            local backend = {
                moduletype = "CharBFBackendStdio";
                args = {(i_0 == bank.primary_idx)};
                biflow_socket = {bind = "&uart_qup_"..tostring(qup_index)..".backend_socket"},
            };
            platform["backend_"..tostring(qup_index)] = backend;
            local qup = {
                moduletype = "qupv3_qupv3_se_wrapper_se0",
                target_socket = {address=bank.addr + (i_0*QUP_PITCH),
                    size=QUP_SIZE, bind="&router.initiator_socket"},
                irq = {bind = "&gic_0.spi_in_"..spi_irq},
            };
            platform["uart_qup_"..tostring(qup_index)] = qup;
            qup_index = qup_index + 1;
        end
    end


    for i=0,(ARM_NUM_CPUS-1) do
        local cpu = {
            moduletype = "QemuCpuArmCortexA76";
            args = {"&platform.qemu_inst"};
            mem = {bind = "&router.target_socket"};
            has_el3 = (ACCEL ~= "hvf");
            has_el2 = false;
            irq_timer_phys_out = {bind = "&gic_0.ppi_in_cpu_"..i.."_"..ARCH_TIMER_NS_EL1_IRQ},
            irq_timer_virt_out = {bind = "&gic_0.ppi_in_cpu_"..i.."_"..ARCH_TIMER_VIRT_IRQ},
            irq_timer_hyp_out = {bind = "&gic_0.ppi_in_cpu_"..i.."_"..ARCH_TIMER_NS_EL2_IRQ},
            irq_timer_sec_out = {bind = "&gic_0.ppi_in_cpu_"..i.."_"..ARCH_TIMER_S_EL1_IRQ},
            gicv3_maintenance_interrupt = {bind = "&gic_0.ppi_in_cpu_"..i.."_25"},
            pmu_interrupt = {bind = "&gic_0.ppi_in_cpu_"..i.."_23"},
            psci_conduit = "smc";
            mp_affinity = (math.floor(i / 8) << 8) | (i % 8);
            start_powered_off = true;
        };
        if (i==0) then
            cpu["rvbar"] = INITIAL_DDR_SPACE_14GB;
            cpu["start_powered_off"] = false;
        end
        platform["cpu_"..tostring(i)]=cpu;

        platform["gic_0"]["irq_out_" .. i] = {bind="&cpu_"..i..".irq_in"}
        platform["gic_0"]["fiq_out_" .. i] = {bind="&cpu_"..i..".fiq_in"}
        platform["gic_0"]["virq_out_" .. i] = {bind="&cpu_"..i..".virq_in"}
        platform["gic_0"]["vfiq_out_" .. i] = {bind="&cpu_"..i..".vfiq_in"}
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

print ("Lua config Finished.");
