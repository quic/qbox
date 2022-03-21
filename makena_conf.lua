-- Virtual platform configuration: Makena SoC, QNX

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

INITIAL_DDR_SPACE_14GB = 0x80000000
OFFSET_MIFS_DDR_SPACE  = 0x20000000
OFFSET_SMEM_DDR_SPACE  = 0x00900000
DDR_SPACE_SIZE = 8*1024*1024*1024 -- 8 GB - should be enough?

UNLIKELY_TO_BE_USED = INITIAL_DDR_SPACE_14GB + DDR_SPACE_SIZE
DTB_LOAD_ADDR = UNLIKELY_TO_BE_USED + 512

APSS_GIC600_GICD_APSS = 0x17A00000
OFFSET_APSS_ALIAS0_GICR_CTLR = 0x60000

-- Makena VBSP, system UART addr: reallocated space at
--   PCIE_3APCIE_WRAPPER_AXI_G3X4_EDMA_AUTO:
PCIE_3APCIE_WRAPPER_AXI_G3X4_EDMA_AUTO = 0x40000000
UART0 = PCIE_3APCIE_WRAPPER_AXI_G3X4_EDMA_AUTO

_KERNEL64_LOAD_ADDR = INITIAL_DDR_SPACE_14GB
dofile (top().."fw/arm64_bootloader.lua")

local hexagon_cluster= {
    hexagon_num_threads = 1;
    hexagon_thread_0={start_powered_off = false};
--    hexagon_thread_1={start_powered_off = true};
--    hexagon_thread_2={start_powered_off = true};
--    hexagon_thread_3={start_powered_off = true};
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
    arm_num_cpus = 1;
    num_redists=1;
    hexagon_num_clusters = 0;
    quantum_ns = 100000000;

    ArmQemuInstance = { tcg_mode="MULTI", sync_policy = "multithread-unconstrained"};
    HexagonQemuInstance = { tcg_mode="SINGLE", sync_policy = "multithread-unconstrained"};

    ram=  {  target_socket = {address=INITIAL_DDR_SPACE_14GB, size=8*1024*1024*1024}};
    hexagon_ram={target_socket={address=UNLIKELY_TO_BE_USED+0x0, size=0x08000000}};
    rom=  {  target_socket = {address=UNLIKELY_TO_BE_USED+0xde000000, size=0x400 },read_only=true};
    gic=  {  dist_iface    = {address=APSS_GIC600_GICD_APSS, size= OFFSET_APSS_ALIAS0_GICR_CTLR};
             redist_iface_0= {address=APSS_GIC600_GICD_APSS+OFFSET_APSS_ALIAS0_GICR_CTLR, size=0xf60000}};
    virtionet0= { mem    =   {address=0x1c120000, size=0x10000}, irq=18, netdev_str="type=user,hostfwd=tcp::2222-:22"};
    virtioblk0= { mem    =   {address=0x1c0d0000, size=0x2000}, irq=9, blkdev_str="file="..top().."fw/fastrpc-images/images/disk.bin,format=raw,if=none"};
    uart= {  simple_target_socket_0 = {address= UART0, size=0x1000}, irq=1};

    ipcc= {  socket        = {address=0x410000, size=0xfc000}};

    smmu = { mem = {address=0x15000000, size=0x100000};
        num_tbu=2;
        upstream_socket_0 = {address=0x0, size=0xd81e0000, relative_addresses=false};
        upstream_socket_1 = {address=0x0, size=0xd81e0000, relative_addresses=false};
    };
    qtb = { control_socket = {address=0x15180000, size=0x80000}};

    fallback_memory = { target_socket={address=0x0, size=0x40000000}, dmi_allow=false, verbose=true, load={csv_file=top().."fw/makena/SA8540P_MakenaAU_v2_Registers.csv", offset=0, addr_str="Address", value_str="Reset Value", byte_swap=true}};
    load={
        {bin_file=top().."fw/makena/images/mifs_qdrive.img", address=INITIAL_DDR_SPACE_14GB + OFFSET_MIFS_DDR_SPACE };
        {bin_file=top().."fw/makena/images/smem_v3.bin", address=INITIAL_DDR_SPACE_14GB + OFFSET_SMEM_DDR_SPACE };
        -- Entry point for bl31.elf should be set to INITIAL_DDR_SPACE_14GB:
        {elf_file=top().."fw/makena/images/bl31.elf"};
    };

    --uart_backend_port=4001;
};


if (platform.arm_num_cpus > 0) then
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


if (platform.hexagon_num_clusters > 0) then
    -- need to set teh SID for each hexagon
    -- stuff from qtb registers ... 84203181 - ony bottom 0x3ff shoudl be used...
    -- e.g. 0x  181 ..2 ..3 ..
    -- SID Composition   SID[14:0] = { TBU_ID[4:0], TopoID[4:0], CSID[4:0] }

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

print ("Lua config run. . . ");
