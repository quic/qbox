-- Virtual platform configuration
-- Commented out parameters show default values

function top()
    local str = debug.getinfo(2, "S").source:sub(2)
    if str:match("(.*/)")
    then
        return str:match("(.*/)")
    else
        return "./"
    end
 end

dofile(top().."../fw/utils.lua");
print ("Lua config running. . . ");

_KERNEL64_LOAD_ADDR =0x00080000
_DTB_LOAD_ADDR =     0x08000000
dofile (top().."fw/config/arm64_bootloader.lua")

local ARCH_TIMER_VIRT_IRQ = 16 + 11
local ARCH_TIMER_S_EL1_IRQ = 16 + 13
local ARCH_TIMER_NS_EL1_IRQ = 16 + 14
local ARCH_TIMER_NS_EL2_IRQ = 16 + 10

local ARM_NUM_CPUS = 4;
local NUM_REDISTS = 1;

platform = {

    quantum_ns = 100000000;

    moduletype="Container";

    -- cpu_1 = { gdb_port = 1234 };

    router = {
        moduletype="router";
    },

    qemu_inst_mgr = {
        moduletype = "QemuInstanceManager";
    },

    qemu_inst= {
        moduletype="QemuInstance";
        args = {"&platform.qemu_inst_mgr", "AARCH64"};
        tcg_mode="MULTI",
        sync_policy = "multithread-unconstrained"
    },

    gic_0=  {
        moduletype = "arm_gicv3",
        args = {"&platform.qemu_inst"},
        dist_iface    = {address=0xc8000000, size= 0x10000, bind = "&router.initiator_socket"};
        redist_iface_0= {address=0xc8010000, size=0x20000, bind = "&router.initiator_socket"};
        num_cpus = ARM_NUM_CPUS,
        -- 64 seems reasonable, but can be up to
        -- 960 or 987 depending on how the gic
        -- is used MUST be a multiple of 32
        redist_region = {ARM_NUM_CPUS / NUM_REDISTS};
        num_spi=64};

    ram_0 = {
        moduletype="gs_memory";
        target_socket = {address=0x00000000, size=0x10000000, bind= "&router.initiator_socket"},
    };

    qemu_pl011 = {
        moduletype = "qemu_pl011",
        args = {"&platform.qemu_inst"};
        mem = {address= 0xc0000000,
                                    size=0x1000, 
                                    bind = "&router.initiator_socket"},
        irq_out = {bind = "&gic_0.spi_in_1"},
    };

    -- See this JIRA ticket https://jira-dc.qualcomm.com/jira/browse/QTOOL-90283
    -- ethoc= {
    --         moduletype = "QemuOpencoresEth",
    --         args = {"&platform.qemu_inst"};
    --         regs = {address=0xc7000000, size=0x54, bind = "&router.initiator_socket"},
    --         desc = {address=0xc7000400, size=0x400, bind = "&router.initiator_socket"},
    --         irq_out={bind = "&gic_0.spi_in_2"},
    --     };

    global_peripheral_initiator_arm_0 = {
        moduletype = "global_peripheral_initiator",
        args = {"&platform.qemu_inst", "&platform.cpu_0"},
        global_initiator = {bind = "&router.target_socket"},
    };

    load={
        moduletype = "loader",
        initiator_socket = {bind = "&router.target_socket"};
        {data=_bootloader_aarch64, address=0x00000000 }; -- align address with rvbar
    }
};

if (ARM_NUM_CPUS > 0) then
    for i=0,(ARM_NUM_CPUS-1) do
        local cpu = {
            moduletype = "cpu_arm_neoverseN1";
            args = {"&platform.qemu_inst"};
            mem = {bind = "&router.target_socket"};
            has_el3 = true;
            has_el3 = false;
            irq_timer_phys_out = {bind = "&gic_0.ppi_in_cpu_"..i.."_"..ARCH_TIMER_NS_EL1_IRQ},
            irq_timer_virt_out = {bind = "&gic_0.ppi_in_cpu_"..i.."_"..ARCH_TIMER_VIRT_IRQ},
            irq_timer_hyp_out = {bind = "&gic_0.ppi_in_cpu_"..i.."_"..ARCH_TIMER_NS_EL2_IRQ},
            irq_timer_sec_out = {bind = "&gic_0.ppi_in_cpu_"..i.."_"..ARCH_TIMER_S_EL1_IRQ},
            psci_conduit = "hvc";
            mp_affinity = (math.floor(i / 8) << 8) | (i % 8);
            start_powered_off = true;
        };
        if (i==0) then
            cpu["rvbar"] = 0x00000000;
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
if os.getenv("QBOX_IMAGE_DIR") == nil then
    IMAGE_DIR = "./"
else
    IMAGE_DIR = os.getenv("QBOX_IMAGE_DIR").."/"
end

if (file_exists(IMAGE_DIR.."conf.lua"))
then
    print ("Running local "..IMAGE_DIR.."conf.lua");
    dofile(IMAGE_DIR.."conf.lua");
else
    print ("A local conf.lua file is required in the directory containing your images. That defaults to the current directory or you can set it using QBOX_IMAGE_DIR\n");
    os.exit(1);
end
