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
-- Function to retrieve the size of a file
function fsize (file)
    local current = file:seek()      -- get current position
    local size = file:seek("end")    -- get file size
    file:seek("set", current)        -- restore position
    return size
end
 
dofile(top().."../fw/utils.lua");
print ("Lua config running. . . ");

 
_OPENSBI_LOAD_ADDR  = 0x80000000;
_KERNEL64_LOAD_ADDR = 0x80200000;
_DTB_LOAD_ADDR      = 0x82800000;
_INITRD_LOAD_ADDR   = 0xa0200000;

 
print ("kernel is loaded at: 0x"..string.format("%x",_KERNEL64_LOAD_ADDR));
print ("dtb is loaded at:    0x"..string.format("%x",_DTB_LOAD_ADDR));
print ("initrd is loaded at: 0x"..string.format("%x",_INITRD_LOAD_ADDR));


dofile (top().."fw/riscv64_bootloader.lua")
 
local RISCV_NUM_CPUS = 5
NUM_GPUS = 0;
 
platform = {
    quantum_ns = 1000000;
    moduletype = "Container";


    router = {
        moduletype="router";
    },
 
    qemu_inst_mgr = {
        moduletype = "QemuInstanceManager";
    },
 
    qemu_inst= {
        moduletype="QemuInstance";
        args = {"&platform.qemu_inst_mgr","RISCV64"};
        tcg_mode="MULTI",
        sync_policy = "multithread-unconstrained"
    },
 
    plic_0=   {
            moduletype = "plic_sifive",
            args = {"&platform.qemu_inst"},
            mem    = {address=0x0c000000, size=0x600000, bind = "&router.initiator_socket"},
            num_sources = 96,
            num_priorities = 7,
            priority_base = 0x0,
            pending_base = 0x1000,
            enable_base = 0x2000,
            enable_stride = 0x80,
            context_base =  0x200000,
            context_stride = 0x1000,
            aperture_size = 0x600000,
            hart_config = "MS,MS,MS,MS,MS",
    };
 
    swi_0=    {
            moduletype = "riscv_aclint_swi",
            args = {"&platform.qemu_inst", "&platform.cpu_3"},
            mem    = {address=0x2000000, size=0x4000, bind = "&router.initiator_socket"};
            num_harts = 5,
        };
 
    mtimer_0= {
            moduletype = "riscv_aclint_mtimer",
            args = {"&platform.qemu_inst"},
            mem    =  {address=0x2004000, size=0x8000, bind = "&router.initiator_socket"},
            timecmp_base = 0x0,
            time_base = 0xbff8,
            provide_rdtime = true,
            aperture_size = 0x10000,
            num_harts = 5,
        };

    ram_0 = {
        moduletype = "gs_memory",
        target_socket  = {address=0x0, size=0x400000000, priority=2048 ,bind = "&router.initiator_socket"},
        reset = { bind = "&reset.reset" },
    };

    uart_0 =  {
        moduletype = "uart_16550",
        args = {"&platform.qemu_inst"},
        mem = {address= 0x10000000, size=0x100, bind = "&router.initiator_socket"},
        irq_out={bind = "&plic_0.irq_in_10"},
        regshift=2,
        baudbase=3686400;
    };

    reset = {
        moduletype = "sifive_test",
        args = {"&platform.qemu_inst"},
        target_socket = {address= 0x100000, size=0x1000, bind = "&router.initiator_socket"},
    };

    virtionet0_0 = {
        moduletype = "virtio_mmio_net",
        args = {"&platform.qemu_inst"};
        mem    =   {address=0x10001000, size=0x1000, bind = "&router.initiator_socket"},
        irq_out = {bind = "&plic_0.irq_in_1"},
        netdev_str="type=user,hostfwd=tcp::2222-:22,hostfwd=tcp::2221-:21,hostfwd=tcp::56283-:56283,hostfwd=tcp::55534-:65534,hostfwd=tcp::55535-:65535"};

    virtioblk_0 = {
        moduletype="virtio_mmio_blk",
        args = {"&platform.qemu_inst"};
        mem = { address = 0x10002000, size = 0x1000, bind = "&router.initiator_socket"},
        irq_out = {bind = "&plic_0.irq_in_2"},
        blkdev_str = "file="..top().."fw/Artifacts/image_ext4.img"..",format=raw,if=none,readonly=off" };
    
    gpex_0 = {
        moduletype = "qemu_gpex";
        args = {"&platform.qemu_inst"};
        bus_master = {bind = "&router.target_socket"};
        pio_iface = { address = 0x3000000, size = 0x10000, bind= "&router.initiator_socket"};
        mmio_iface = { address = 0x40000000, size = 0x40000000, bind= "&router.initiator_socket" };
        ecam_iface = { address = 0x30000000, size = 0x10000000, bind= "&router.initiator_socket" };
        mmio_iface_high = { address = 0x400000000, size = 0x400000000, bind= "&router.initiator_socket" },
        irq_out_0 = {bind = "&plic_0.irq_in_32"};
        irq_out_1 = {bind = "&plic_0.irq_in_33"};
        irq_out_2 = {bind = "&plic_0.irq_in_34"};
        irq_out_3 = {bind = "&plic_0.irq_in_35"};
    };

    global_peripheral_initiator_riscv_0 = {
        moduletype = "global_peripheral_initiator",
        args = {"&platform.qemu_inst", "&platform.cpu_0"},
        global_initiator = {bind = "&router.target_socket"},
    };
    
 
    load = {
        moduletype = "loader",
        initiator_socket = {bind = "&router.target_socket"};
        { bin_file=top().."fw/Artifacts/fw_jump.bin", address=_OPENSBI_LOAD_ADDR };
        { bin_file=top().."fw/Artifacts/Image.bin", address=_KERNEL64_LOAD_ADDR };
        { bin_file=top().."fw/Artifacts/ubuntu.dtb", address=_DTB_LOAD_ADDR };
        { bin_file=top().."fw/Artifacts/image_ext4_initrd.img", address= _INITRD_LOAD_ADDR };
        {data=_bootloader_riscv64, address=0x10040 }; -- bootloader 
        {data={0x0400f06f}, address=0x00001000}; -- resetvec_rom_0
        {data={2}, address=0xa0000010}; -- boot_gpio_0
        reset = { bind = "&reset.reset" };
    };
};

platform["with_gpu"] = true;

if (platform.with_gpu == true) then
    if (NUM_GPUS > 0) then
        for i=0,(NUM_GPUS-1) do
            if (i==0) then
                gpu = {
                    moduletype = "virtio_gpu_gl_pci";
                    args = {"&platform.qemu_inst", "&platform.gpex_0"};
                }
            else
                gpu = {
                    moduletype = "virtio_gpu_pci";
                    args = {"&platform.qemu_inst", "&platform.gpex_0"};
                }
            end
            platform["gpu_"..tostring(i)]=gpu;
            display = {
                moduletype = "display";
                args = {"&platform.gpu_"..tostring(i)};
            }
            platform["display_"..tostring(i)]=display;
        end
    end
end

 
if (RISCV_NUM_CPUS > 0) then
    for i=0,(RISCV_NUM_CPUS-1) do
        local cpu = {
            moduletype = "cpu_riscv64";
            args = {"&platform.qemu_inst", i};
            mem = {bind = "&router.target_socket"};
            reset = { bind = "&reset.reset" };
        };
        platform["cpu_"..tostring(i)]=cpu;
    end
end
