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

print ("Lua config running. . . ");

 -- to use the hypervisor:
 --KERNEL64_LOAD_ADDR (0x40200000)
 --DTB_LOAD_ADDR      (0x44000000)

_KERNEL64_LOAD_ADDR =0x41080000
_DTB_LOAD_ADDR =     0x44200000
dofile (top().."fw/arm64_bootloader.lua")


local hexagon_cluster= {
    hexagon_num_threads = 4;
    hexagon_thread_0={start_powered_off = false};
    hexagon_thread_1={start_powered_off = true};
    hexagon_thread_2={start_powered_off = true};
    hexagon_thread_3={start_powered_off = true};
    HexagonQemuInstance = { tcg_mode="SINGLE", sync_policy = "multithread-unconstrained"};
    hexagon_start_addr = 0x8B500000;
    l2vic={  mem           = {address=0xfc910000, size=0x1000};
             fastmem       = {address=0xd81e0000, size=0x10000}};
    qtimer={ mem           = {address=0xfab20000, size=0x1000};
             timer0_mem    = {address=0xfc921000, size=0x1000};
             timer1_mem    = {address=0xfc922000, size=0x1000}};
    tbu = {target_socket   = {address=0x0       , size=0xd81e0000}};
};

platform = {
    arm_num_cpus = 8;
    quantum_ns = 100000000;

    ArmQemuInstance = { tcg_mode="MULTI", sync_policy = "multithread-unconstrained"};

-- cpu_0 = { gdb_port = 1234 };
-- hexagon_0 = { gdb_port = 1234 };


    ram=  {  target_socket = {address=0x40000000, size=0x981E0000}};
    hexagon_ram={target_socket={address=0x0, size=0x08000000}};
    rom=  {  target_socket = {address=0xde000000, size=0x400 },read_only=true};
    gic=  {  dist_iface    = {address=0x17100000, size=0x10000 };
             redist_iface_0= {address=0x171a0000, size=0xf60000}};
    uart= {  mem           = {address= 0x9000000, size=0x1000}};
    ipcc= {  socket        = {address=  0x410000, size=0xfc000}};
    vendor={ target_socket = {address=0x10000000, size=0x20000000}, load={bin_file=top().."fw/fastrpc-images/images/vendor.squashfs", offset=0}};
    system={ target_socket = {address=0x30000000, size=0x10000000}, load={bin_file=top().."fw/fastrpc-images/images/system.squashfs", offset=0}};

    fallback_memory = { target_socket={address=0x0, size=0x10000000}, dmi_allow=false, verbose=true, load={csv_file=top().."fw/SM8450_Waipio.csv", offset=0, addr_str="Address", value_str="Reset Value", byte_swap=true}};

    hexagon_num_clusters = 2;
    hexagon_cluster_0 = hexagon_cluster;
    hexagon_cluster_1 = hexagon_cluster;

    load={
        {bin_file=top().."fw/fastrpc-images/images/Image",    address=_KERNEL64_LOAD_ADDR};
        {bin_file=top().."fw/fastrpc-images/images/rumi.dtb", address=_DTB_LOAD_ADDR};
        {data=_bootloader_aarch64, address = 0x40000000};

        {elf_file=top().."fw/hexagon-images/bootimage_kailua.cdsp.coreQ.pbn"};

        -- for hypervisor        {elf_file=top().."fw/fastrpc-images/images/hypvm.elf"};

        {data={0x1}, address = 0x30}; -- isdb_secure_flag
        {data={0x1}, address = 0x34}; -- isdb_trusted_flag
    }
};


if (platform.arm_num_cpus) then
    for i=0,(platform.arm_num_cpus-1) do
        local cpu = {
            has_el3 = true;
            has_el3 = false;
            psci_conduit = "smc";
            mp_affinity = (math.floor(i / 8) << 8) | (i % 8);
            start_powered_off = true;
        };
        if (i==0) then
            cpu["rvbar"] = 0x40000000;
            cpu["start_powered_off"] = false;
        end
        platform["cpu_"..tostring(i)]=cpu;
    end
end


if (platform.num_hexagon_clusters) then
    platform["cfgTable"] = {
        fastl2vic_base = platform.hexagon_cluster_0.l2vic.fastmem.address,
    };

    platform["cfgExtensions"] = {
        cfgtable_size = platform.rom.target_socket.size,
        l2vic_base = platform.hexagon_cluster_0.l2vic.mem.address,
        qtmr_rg0 = platform.hexagon_cluster_0.qtimer.timer0_mem.address,
        qtmr_rg1 = platform.hexagon_cluster_0.qtimer.timer1_mem.address,
    };
end
