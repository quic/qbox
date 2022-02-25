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



_KERNEL64_LOAD_ADDR =0x41080000
_DTB_LOAD_ADDR =     0x44200000
dofile (top().."fw/arm64_bootloader.lua")


local hexagon_cluster= {
    hexagon_num_threads = 1;
    hexagon_thread_0={start_powered_off = false};
    -- hexagon_thread_1={start_powered_off = true};
    -- hexagon_thread_2={start_powered_off = true};
    -- hexagon_thread_3={start_powered_off = true};
    HexagonQemuInstance = { tcg_mode="SINGLE", sync_policy = "multithread-unconstrained"};
    hexagon_start_addr = 0x8B500000;
    l2vic={  mem           = {address=0xfc910000, size=0x1000};
             fastmem       = {address=0xd81e0000, size=0x10000}};
    qtimer={ mem           = {address=0xfab20000, size=0x1000};
             timer0_mem    = {address=0xfc921000, size=0x1000};
             timer1_mem    = {address=0xfc922000, size=0x1000}};
};

platform = {
    arm_num_cpus = 4;
    num_redists=1;
    quantum_ns = 100000000;

    ArmQemuInstance = { tcg_mode="MULTI", sync_policy = "multithread-unconstrained"};

    ram=  {  target_socket = {address=0x40000000, size=0x981E0000}};
    hexagon_ram={target_socket={address=0x0, size=0x08000000}};
    rom=  {  target_socket = {address=0xde000000, size=0x400 },read_only=true};
    gic=  {  dist_iface    = {address=0x17100000, size=0x10000 };
             redist_iface_0= {address=0x171a0000, size=0xf60000}};
    uart= {  simple_target_socket_0           = {address= 0x9000000, size=0x1000}, irq=1};
    ipcc= {  socket        = {address=  0x410000, size=0xfc000}};
    virtionet0= { mem    =   {address=0x0a003e00, size=0x2000}, irq=2}; -- netdev_str="type=tap"};
    virtioblk0= { mem    =   {address=0x0a003c00, size=0x2000}, irq=3, blkdev_str="file="..top().."fw/fastrpc-images/images/disk.bin,format=raw,if=none"};

    system_imem={ target_socket = {address=0x14680000, size=0x40000}};

    fallback_memory = { target_socket={address=0x00000000, size=0x40000000}, dmi_allow=false, verbose=true, load={csv_file=top().."fw/SM8450_Waipio.csv", offset=0, addr_str="Address", value_str="Reset Value", byte_swap=true}};

    hexagon_num_clusters = 1;
    hexagon_cluster_0 = hexagon_cluster;
    --hexagon_cluster_1 = hexagon_cluster;
    smmu = { mem = {address=0x15000000, size=0x100000};
             num_tbu=2;
             upstream_socket_0 = {address=0x0, size=0xd81e0000, relative_addresses=false};
             upstream_socket_1 = {address=0x0, size=0xd81e0000, relative_addresses=false};
            };
    qtb = { control_socket = {address=0x15180000, size=0x4000}}; -- + 0x4000*tbu number

    load={
        {bin_file=top().."fw/fastrpc-images/images/Image",    address=_KERNEL64_LOAD_ADDR};
        {bin_file=top().."fw/fastrpc-images/images/rumi.dtb", address=_DTB_LOAD_ADDR};
        {data=_bootloader_aarch64, address = 0x40000000};

        {elf_file=top().."fw/hexagon-images/bootimage_kailua.cdsp.coreQ.pbn"};

        -- for hypervisor        {elf_file=top().."fw/fastrpc-images/images/hypvm.elf"};

        {data={0x1}, address = 0x30}; -- isdb_secure_flag
        {data={0x1}, address = 0x34}; -- isdb_trusted_flag
    };

--    uart_backend_port=4001;
};


if (platform.arm_num_cpus > 0) then
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


if (platform.hexagon_num_clusters > 0) then
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
