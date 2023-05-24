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

print ("Lua config running. . . ");

_KERNEL_FILE = top().."fw/images/Image"

local kernel_file = io.open(_KERNEL_FILE,"r")
_KERNEL_FILE_SIZE = fsize(kernel_file);
kernel_file:close()

_OPENSBI_LOAD_ADDR = 0x70000000
_KERNEL64_LOAD_ADDR =0x1000000000
_DTB_ALIGN = 2 * 1024 * 1024;
_DTB_LOAD_ADDR = ((_KERNEL64_LOAD_ADDR + _KERNEL_FILE_SIZE)
                        & ~(_DTB_ALIGN - 1)) + _DTB_ALIGN;

dofile(top().."../fw/utils.lua");
dofile (top().."fw/config/riscv64_bootloader.lua")

platform = {
    riscv_num_cpus = 5;
    quantum_ns = 100000000;

    RISCVQemuInstance = { tcg_mode="MULTI", sync_policy = "multithread"};

    -- cpu_0 = { gdb_port = 1234 };

    plic=   { mem    = {address=0x0c000000, size=0x4000000 }};
    swi=    { mem    = {address=0x2000000, size=0x4000 }};
    mtimer= { mem    =  {address=0x2004000, size=0x8000 }};
    resetvec_rom= { target_socket  = {address=0x00001000, size=0x1000}, load={data={0x0400f06f}, offset=0}};
    rom=   { target_socket  = {address=0x10000, size=64 * 1024}};
    sram=   { target_socket  = {address=0x60000000, size=128 * 1024}};
    dram=   { target_socket  = {address=0x1000000000, size=256 * 1024 * 1024}};
    qspi=   { target_socket  = {address=0x70000000, size=256 * 1024 * 1024}};
    boot_gpio={ target_socket  = {address=0xa0000010, size=0x08}, load= {data={2}, offset=0}};
    uart=  { mem = {address= 0x54000000, size=0x10000}, irq=273};

    fallback_memory = { target_socket={address=0x0, size=0x100000000}, dmi_allow=true, verbose=true};

    load={
        {data=_bootloader_riscv64, address=0x10040 };
    }
};

if (platform.riscv_num_cpus > 0) then
    for i=0,(platform.riscv_num_cpus-1) do
        platform["cpu_"..tostring(i)]=cpu;
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
    print ("A local conf.lua file is required in the directory containing your images. That defaults to the current directory or you can set it using QQVP_IMAGE_DIR\n");
    os.exit(1);
end
