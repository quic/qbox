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

-- read in any local configuration (Before convenience switches)
local IMAGE_DIR;
if os.getenv("QBOX_IMAGE_DIR") == nil then
    IMAGE_DIR = "./"
else
    IMAGE_DIR = os.getenv("QBOX_IMAGE_DIR").."/"
end

_KERNEL_FILE = IMAGE_DIR.."images/Image"

local kernel_file = io.open(_KERNEL_FILE,"r")
_KERNEL_FILE_SIZE = fsize(kernel_file);
kernel_file:close()

_OPENSBI_LOAD_ADDR = 0x70000000
_KERNEL64_LOAD_ADDR =0x1000000000
_DTB_ALIGN = 2 * 1024 * 1024;
_DTB_LOAD_ADDR = ((_KERNEL64_LOAD_ADDR + _KERNEL_FILE_SIZE)
                        & ~(_DTB_ALIGN - 1)) + _DTB_ALIGN;

dofile (top().."fw/config/riscv64_bootloader.lua")

local RISCV_NUM_CPUS = 5;

platform = {
    quantum_ns = 100000000;

    moduletype = "Container";

    -- cpu_0 = { gdb_port = 1234 };

    router = {
        moduletype="router";

    },

    qemu_inst_mgr = {
        moduletype = "QemuInstanceManager";
    },

    qemu_inst= {
        moduletype="QemuInstance";
        args = {"&platform.qemu_inst_mgr", "RISCV64"};
        tcg_mode="MULTI",
        sync_policy = "multithread"
    },

    plic_0=   {
            moduletype = "plic_sifive",
            args = {"&platform.qemu_inst"},
            mem    = {address=0x0c000000, size=0x4000000, bind = "&router.initiator_socket"},
            num_sources = 280,
            num_priorities = 7,
            priority_base = 0x04,
            pending_base = 0x1000,
            enable_base = 0x2000,
            enable_stride = 0x80,
            context_base = 0x200000,
            context_stride = 0x1000,
            aperture_size = 0x4000000,
            hart_config = "MS,MS,MS,MS,MS"
    };

    swi_0=    {
            moduletype = "riscv_aclint_swi",
            args = {"&platform.qemu_inst", "&platform.cpu_4"},
            mem    = {address=0x2000000, size=0x4000, bind = "&router.initiator_socket"};
            num_harts = 5
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
    
    resetvec_rom_0= {
            moduletype = "gs_memory",
            target_socket  = {address=0x00001000, size=0x1000, bind = "&router.initiator_socket"},
            load={data={0x0400f06f}, offset=0}
        };
    
    rom_0=   {
            moduletype = "gs_memory",
            target_socket  = {address=0x10000, size=64 * 1024, bind = "&router.initiator_socket"}
        };
    
    sram_0=   {
            moduletype = "gs_memory",
            target_socket  = {address=0x60000000, size=128 * 1024, bind = "&router.initiator_socket"}
        };
    
    dram_0=   {
            moduletype = "gs_memory",
            target_socket  = {address=0x1000000000, size=256 * 1024 * 1024, bind = "&router.initiator_socket"}
        };
    
    qspi_0=   {
            moduletype = "gs_memory",
            target_socket  = {address=0x70000000, size=256 * 1024 * 1024, bind = "&router.initiator_socket"}
        };
    
    boot_gpio_0={
            moduletype = "gs_memory",
            target_socket  = {address=0xa0000010, size=0x08, bind = "&router.initiator_socket"},
            load= {data={2},offset=0}
        };
    
    uart_0=  {
            moduletype = "uart_16550",
            args = {"&platform.qemu_inst"},
            mem = {address= 0x54000000, size=0x10000, bind = "&router.initiator_socket"},
            irq_out={bind = "&plic_0.irq_in_273"},
    };

    -- fallback_memory_0 = {
    --     moduletype="Memory";
    --     target_socket={size=0x100000000, dynamic=true, bind="&router.initiator_socket"},
    --     dmi_allow=false,
    --     verbose=true
    -- };

    load={
        moduletype = "loader",
        initiator_socket = {bind = "&router.target_socket"};
        {data=_bootloader_riscv64, address=0x10040 };
    }
};

if (RISCV_NUM_CPUS > 0) then
    for i=0,(RISCV_NUM_CPUS-1) do
        local cpu = {
            moduletype = "cpu_riscv64";
            args = {"&platform.qemu_inst", i};
            mem = {bind = "&router.target_socket"};
        };
        platform["cpu_"..tostring(i)]=cpu;
    end
end

-- -- read in any local configuration (Before convenience switches)
-- local IMAGE_DIR;
-- if os.getenv("QBOX_IMAGE_DIR") == nil then
--     IMAGE_DIR = "./"
-- else
--     IMAGE_DIR = os.getenv("QBOX_IMAGE_DIR").."/"
-- end

if (file_exists(IMAGE_DIR.."conf.lua"))
then
    print ("Running local "..IMAGE_DIR.."conf.lua");
    dofile(IMAGE_DIR.."conf.lua");
else
    print ("A local conf.lua file is required in the directory containing your images. That defaults to the current directory or you can set it using QBOX_IMAGE_DIR\n");
    os.exit(1);
end
