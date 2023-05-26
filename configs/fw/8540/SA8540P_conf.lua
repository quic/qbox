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

local MAKENA_REGS_CSV = valid_file(top().."8540_Registers.csv")
local QDSP6_CFG   = valid_file(top().."../qdsp6.lua")

local NSP0_BASE     = 0x1A000000 -- TURING_SS_0TURING
local NSP1_BASE     = 0x20000000 -- TURING_SS_1TURING
   -- ^^ This val is also described as 'TCM_BASE_ADDRESS' in
   -- Sys arch spec "TCM region memory map"
-- local NSP0_AHBS_BASE= 0x1B300000 -- TURING_SS_0TURING_QDSP6V68SS
-- local NSP1_AHBS_BASE= 0x21300000 -- TURING_SS_0TURING_QDSP6V68SS
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

local UART0 = 0x10000000

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

dofile(QDSP6_CFG);

-- NSP0 configuration --
local SA8540P_nsp0_config_table= get_SA8540P_nsp0_config_table();
local nsp0ss = get_dsp(
        "v68",
        0x1A000000, -- TURING_SS_0TURING
        0x1B300000, -- TURING_SS_0TURING_QDSP6V68SS
        SA8540P_nsp0_config_table,
        0x89F00000, -- entry point address from bootimage_makena.cdsp0.prodQ.pbn
        NSP0_AHB_SIZE,
        6 -- threads
        );

assert((SA8540P_nsp0_config_table[11] << 16) == nsp0ss.l2vic.fastmem.address)
-- So far, nothing depends on _csr, but a good sanity check:
assert((SA8540P_nsp0_config_table[3] << 16) == TURING_SS_0TURING_QDSP6V68SS_CSR)
local NSP0_VTCM_BASE_ADDR = (SA8540P_nsp0_config_table[15] << 16)
local NSP0_VTCM_SIZE_BYTES = (SA8540P_nsp0_config_table[16] * 1024)

-- NSP1 configuration --
local SA8540P_nsp1_config_table= get_SA8540P_nsp1_config_table();
local nsp1ss = get_dsp(
        "v68",
        0x20000000, -- TURING_SS_1TURING
        0x21300000, -- TURING_SS_1TURING_QDSP6V68SS
        SA8540P_nsp1_config_table,
        0x8C600000, -- entry point address from bootimage_makena.cdsp1.prodQ.pbn
        NSP1_AHB_SIZE,
        6 -- threads
        );
assert((SA8540P_nsp1_config_table[11] << 16) == nsp1ss.l2vic.fastmem.address)
assert((SA8540P_nsp1_config_table[3] << 16) == TURING_SS_1TURING_QDSP6V68SS_CSR)
local NSP1_VTCM_BASE_ADDR = (SA8540P_nsp1_config_table[15] << 16)
local NSP1_VTCM_SIZE_BYTES = (SA8540P_nsp1_config_table[16] * 1024)

-- these values are helpful for local configuration, so make them global
 OFFSET_MIFS_DDR_SPACE  = 0x20000000
 OFFSET_SMEM_DDR_SPACE  = 0x00900000
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

local ARM_NUM_CPUS = 8;
local NUM_REDISTS = 1;
local HEXAGON_NUM_CLUSTERS = 2;

zipfile = valid_file(top().."../top.zip");

platform = {
    with_gpu = false;

    hexagon_num_clusters = 2;
    quantum_ns = 10000000;

    moduletype="Container";


    hexagon_cluster_0 = nsp0ss;
    hexagon_cluster_1 = nsp1ss;

    router = {
        moduletype="Router";
        log_level=0;
        -- target_socket = {bind = "&tbu_2.downstream_socket"};
    },

    DDR_space = {moduletype="Memory";
                target_socket = {bind = "&router.initiator_socket";}
                };
    
    -- ram_0 = {
    --     moduletype="Memory";
    --     target_socket = {"&platform.DDR_space", bind= "&router.initiator_socket"},
    --     -- target_socket= {size = DDR_SPACE_SIZE/2,
    --     --                 address=INITIAL_DDR_SPACE_14GB;
    --     --                 bind = "&router.initiator_socket";}
    -- },
    DDR_space_1 = {moduletype="Memory";
                target_socket = {bind = "&router.initiator_socket";}
                };

    -- ram_1 = {
    --     moduletype="Memory";
    --     target_socket= {size = DDR_SPACE_SIZE/2,
    --                     address=INITIAL_DDR_SPACE_14GB+(DDR_SPACE_SIZE/2);
    --                     bind = "&router.initiator_socket";}
    -- },

    hexagon_ram_0 = {
        moduletype="Memory";
        target_socket= {size = NSP0_VTCM_SIZE_BYTES,
                        address=NSP0_VTCM_BASE_ADDR;
                        bind = "&router.initiator_socket";}
    },
    hexagon_ram_1 = {
        moduletype="Memory";
        target_socket= {size = NSP1_VTCM_SIZE_BYTES,
                        address=NSP1_VTCM_BASE_ADDR;
                        bind = "&router.initiator_socket";}
    },

    qemu_inst_mgr = {
        moduletype = "SC_QemuInstanceManager";
    },

    qemu_inst_mgr_h = {
        moduletype = "SC_QemuInstanceManager";
    },

    qemu_inst= {
        moduletype="SC_QemuInstance";
        args = {"&platform.qemu_inst_mgr", "AARCH64"};
        QemuInstance = {tcg_mode="MULTI", sync_policy = "multithread-unconstrained"};
    },


    gic_0=  {
                moduletype = "QemuArmGicv3",
                args = {"&platform.qemu_inst"},
                dist_iface    = {address=APSS_GIC600_GICD_APSS, size= OFFSET_APSS_ALIAS0_GICR_CTLR, bind = "&router.initiator_socket"};
                redist_iface_0= {address=APSS_GIC600_GICD_APSS+OFFSET_APSS_ALIAS0_GICR_CTLR, size=0x1C0000, bind = "&router.initiator_socket"};
                num_cpus = ARM_NUM_CPUS,
                -- 64 seems reasonable, but can be up to
                -- 960 or 987 depending on how the gic
                -- is used MUST be a multiple of 32
                redist_region = {ARM_NUM_CPUS / NUM_REDISTS};
                num_spi=960};
    virtionet0_0= { 
                    moduletype = "QemuVirtioMMIONet",
                    args = {"&platform.qemu_inst"};
                    mem    =   {address=0x1c120000, size=0x10000, bind = "&router.initiator_socket"},
                    irq_out = {bind = "&gic_0.spi_in_18"},
                    netdev_str="type=user,hostfwd=tcp::2222-:22,hostfwd=tcp::2221-:21",
    };
--    virtioblk_0= { mem    =   {address=0x1c0d0000, size=0x2000}, 
--                     irqs = {
--                         {irq=9, dst = "platform.gic.spi_in_9"},
--                     };
--                     blkdev_str="file="..SYSTEM_QDRIVE..",format=raw,if=none",
--     };

    qtimer_0=   { 
        moduletype = "QemuHexagonQtimer",
        args = {"&platform.qemu_inst"};
        mem  =   {address=0x17c20000, size=0x1000, bind = "&router.initiator_socket"};
        mem_view=   {address=0x17c21000, size=0x1000*2*7, bind = "&router.initiator_socket"};
        irq_0 = {bind = "&gic_0.spi_in_40"},
        irq_1 = {bind = "&gic_0.spi_in_41"},
        irq_2 = {bind = "&gic_0.spi_in_42"},
        irq_3 = {bind = "&gic_0.spi_in_43"},
        irq_4 = {bind = "&gic_0.spi_in_44"},
        irq_5 = {bind = "&gic_0.spi_in_45"},
        irq_6 = {bind = "&gic_0.spi_in_46"},
        nr_frames=7;nr_views=2;cnttid=0x1111515};

    charbackend_stdio_0 = {
        moduletype = "CharBackendStdio";
        args = {false};
    };

    pl011 = {
        moduletype = "Pl011",
        args = {"&platform.charbackend_stdio_0"};
        simple_target_socket_0 = {address= UART0,
                                  size=0x1000, 
                                  bind = "&router.initiator_socket"},
        irq = {bind = "&gic_0.spi_in_379"},
    };

    ipc_router_top = {
            moduletype = "IPCC",
            target_socket = {bind = "&router.initiator_socket"},
            irq_8 = {bind = "&gic_0.spi_in_229"},
            irq_18 = {bind = "&hexagon_cluster_1.l2vic.irq_in_30"},
            irq_6 = {bind = "&hexagon_cluster_0.l2vic.irq_in_30"},
        };
    mpm = { socket = {address=0x0C210000, size=0x1000}};

    gpex_0 = {
        moduletype = "QemuGPEX";
        args = {"&platform.qemu_inst"};
        bus_master = {"&router.target_socket"};
        pio_iface = { address = 0x003eff0000, size = 0x0000010000, bind= "&router.initiator_socket"};
        mmio_iface = { address = 0x0060000000, size = 0x002B500000, bind= "&router.initiator_socket"};
        ecam_iface = { address = 0x4010000000, size = 0x0010000000, bind= "&router.initiator_socket"};
        mmio_iface_high = { address = 0x8000000000, size = 0x8000000000, bind= "&router.initiator_socket"};
        irq_out_0 = {bind = "&gic_0.spi_in_0"};
        irq_out_1 = {bind = "&gic_0.spi_in_0"};
        irq_out_2 = {bind = "&gic_0.spi_in_0"};
        irq_out_3 = {bind = "&gic_0.spi_in_0"};
        irq_num_0 = 0;
        irq_num_1 = 0;
        irq_num_2 = 0;
        irq_num_3 = 0;
    };

    -- qtb = { control_socket = {address=0x15180000, size=0x80000}};
    -- The QTB model is not active in the system, left here for debug purposes.

    tbu_0 = {
              moduletype = "smmu500_tbu",
              args = {"&platform.smmu_0"},
              topology_id=0x31A0,
              upstream_socket = { address=NSP0_AHB_HIGH,
                                  size=0xF00000000-NSP0_AHB_HIGH,
                                  relative_addresses=false,
                                  bind = "&hexagon_cluster_0.router.initiator_socket"
             },
             downstream_socket = {bind = "&router.target_socket"};
            };
    tbu_1 = {
             moduletype = "smmu500_tbu",
             args = {"&platform.smmu_0"},
             topology_id=0x39A0,
             upstream_socket = { address=NSP1_AHB_HIGH,
                                 size=0xF00000000-NSP1_AHB_HIGH,
                                 relative_addresses=false,
                                 bind = "&hexagon_cluster_1.router.initiator_socket"
             },
             downstream_socket = {bind = "&router.target_socket"};
            };

    -- tbu_2 = {
    --     moduletype = "smmu500_tbu",
    --     args = {"&platform.smmu_0"},
    --     topology_id=0x39A0,
    --     upstream_socket = { address=NSP1_AHB_HIGH,
    --                         size=0xF00000000-NSP1_AHB_HIGH,
    --                         relative_addresses=false,
    --                         bind = "&remote_0.initiator_socket_0"
    --     -- downtream_socket = {bind = "router.target_socket"};
    --     }
    --    };

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

    global_peripheral_initiator_arm_0 = {
                   moduletype = "GlobalPeripheralInitiator",
                   args = {"&platform.qemu_inst", "&platform.cpu_0"},
                   global_initiator = {bind = "&router.target_socket"},
    };

    load={
        moduletype = "Loader",
        initiator_socket = {bind = "&router.target_socket"};
        {data={0x60140200}, address=TCSR_SOC_HW_VERSION_ADDR};
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


        -- GCC_QUPV3_WRAP0_S3_CFG_RCGR
        {data={0x2601}, address=0x1174dc};
        -- GCC_QUPV3_WRAP0_S3_M
        {data={0x180}, address=0x1174e0};
        -- GCC_QUPV3_WRAP0_S3_N
        {data={0xc476}, address=0X1174e4};
        -- GCC_QUPV3_WRAP0_S3_D
        {data={0xc2f6}, address=0x1174e8};
--      {data={0x00020400}, address=0x10E0000};
--      {data={0x60000003}, address=0x10E000C};
    };
};

local irq_context = 103
local irq_global = 65
local num_cb = 128

local smmu_index = 0;

for i = 0, num_cb - 1 do
    local smmu = {
        moduletype = "smmu500",
        dma = {bind = "&router.target_socket"},
        target_socket = {"&platform.sys_tcu_cfg_v2smmu_500_apps_reg_wrapper", bind= "&router.initiator_socket"},
        num_tbu = 2,
        num_pages = 128,
        num_cb = 128;
        num_smr = 224,
        irq_global = {bind = "&gic_0.spi_in_"..irq_global},
    }
    smmu["irq_context_" .. i] = {bind = "&gic_0.spi_in_" .. irq_context + i}
    platform["smmu_"..tostring(smmu_index)] = smmu;
end

if (ARM_NUM_CPUS > 0) then
    local QUP_PITCH = 0x4000;
    local QUP_SIZE = 0x2000;
    local QUP_BANKS = {
        -- QUPV3_2 / u_qupv3_wrapper_2 / qupv3_se_irq[*]:
        { addr = 0x880000, primary_idx = 1,
          qgic_spi_irqs = { 373, 583, 584, 585, 586, 587, 834, 835, },
        },
        -- QUPV3_0 / u_qupv3_wrapper_0 / qupv3_se_irq[*]:
        { addr = 0x980000, primary_idx = nil,
          qgic_spi_irqs = {633, 634, 635, 636, 637, 638, 639, 640, },
        },
        -- QUPV3_1 / u_qupv3_wrapper_1 / qupv3_se_irq[*]:
        { addr = 0xa80000, primary_idx = nil,
          qgic_spi_irqs = { 353, 354, 355, 356, 357, 358, 836, 837, },
        },
    };

    local qup_index = 0;
    for _, bank in next, QUP_BANKS do
        -- assert(bank.count == #bank.qgic_spi_irqs)
        for i, spi_irq in next, bank.qgic_spi_irqs do
            local i_0 = i - 1;
            local backend = {
                moduletype = "CharBackendStdio";
                args = {(i_0 == bank.primary_idx)};
            };
            platform["backend_"..tostring(qup_index)] = backend;
            local qup = {
                moduletype = "QUPv3",
                args = {"&platform.backend_"..tostring(qup_index)};
                simple_target_socket_0 = {address=bank.addr + (i_0*QUP_PITCH),
                    size=QUP_SIZE, bind="&router.initiator_socket"},
                irq = {bind = "&gic_0.spi_in_"..spi_irq},
            };
            platform["uart_qup_"..tostring(qup_index)] = qup;
            qup_index = qup_index + 1;
        end
    end

    -- for i=0,(HEXAGON_NUM_CLUSTERS-1) do
    --     platform.router["target_socket"] = {bind = "&tbu_"..i.."_downstream_socket"};
    -- end

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
            cpu["rvbar"] = platform.ram_0.target_socket.address;
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
