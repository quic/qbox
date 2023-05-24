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

platform = {
    arm_num_cpus = 4;
    quantum_ns = 100000000;
    
    ArmQemuInstance = { tcg_mode="MULTI", sync_policy = "multithread-unconstrained"};

    -- cpu_0 = { gdb_port = 1234 };

    gic=   { dist_iface     = {address=0xc8000000, size=0x10000 };
             redist_iface_0 = {address=0xc8010000, size=0x20000 }};
    ram=   { target_socket  = {address=0x00000000, size=0x10000000}};
    uart=  { mem = {address= 0xc0000000, size=0x1000}, irq=1};

    fallback_memory = { target_socket={address=0x0, size=0x100000000}, dmi_allow=false, verbose=true};

    load={
        {data=_bootloader_aarch64, address=0x00000000 }; -- align address with rvbar
        {data={0x30001000, -- stack pointer
               0x30000000} -- reset pointer
                            , address = 0x20000000}; -- m7 reset vectors
    }
};

if (platform.arm_num_cpus > 0) then
    for i=0,(platform.arm_num_cpus-1) do
        local cpu = {
            has_el3 = true;
            has_el3 = false;
            psci_conduit = "hvc";
            mp_affinity = (math.floor(i / 8) << 8) | (i % 8);
            start_powered_off = true;
        };
        if (i==0) then
            cpu["rvbar"] = 0x00000000;
            cpu["start_powered_off"] = false;
        end
        platform["cpu_"..tostring(i)]=cpu;
    end
end

-- platform["ArmQemuInstance.args.-netdev"]={}
-- platform["ArmQemuInstance.args.-netdev"]="type=user,id=gs_net_id"

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
