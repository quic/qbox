-- Virtual platform configuration

-- IMAGES USED

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

function installdir(filename)
    local install_dir = "/prj/qct/llvm/release/internal/QEMU-VP/branch-1.0/linux64/latest"
    _dir = io.open(install_dir..filename, "r")
    if _dir then
        return install_dir
    else
        return
    end
end

--
-- Set the pre-installed image dir here.
--   - This can point to the location of externally built images.
--
function image_install_dir()
    return "/prj/qct/llvm/target/vp_qemu_llvm/images/gki/"
end

--
-- Find the images, either one that has been installed or a locally
-- built image.
--
function get_image(local_path, install_path, base_name)
    target_image = top() .. local_path
    if io.open(target_image, "r") then
        target_image = target_image
    else
        target_image = install_path
        if (target_image == nil or io.open(target_image, "r") == nil) then
          print ("ERROR: File \"" .. base_name .."\" not found")
          return nil
        end
    end
    return target_image
end


filesystem_image = get_image("bsp/linux/extras/fs/filesystem.bin",
                             image_install_dir().."filesystem.bin",
                             "filesystem.bin")

linux_image = get_image ("fw/fastrpc-images/images/Image_opengl",
                         nil,
                         "Image_opengl");

device_tree = get_image ("fw/fastrpc-images/images/rumi_opengl.dtb",
                         nil,
                         "rumi_opengl.dtb");

-- print (linux_image)
-- print (device_tree)
-- print (filesystem_image)

if (linux_image == nil or device_tree == nil or filesystem_image == nil) then
    return
end


_KERNEL64_LOAD_ADDR =0x41080000
_DTB_LOAD_ADDR =     0x44200000
dofile (top().."fw/arm64_bootloader.lua")

local NSP0_AHBS_BASE= 0x1B300000 -- TURING_SS_0TURING_QDSP6V68SS
local NSP0_BASE     = 0x1A000000 -- TURING_SS_0TURING
local CFGTABLE_BASE  = NSP0_BASE + 0x180000;

local SA8540P_nsp0_config_table =  {
    --/* captured from an SA840P-NSP0 via T32 */
        0x00001a00,   -- .l2tcm_base
        0x00000000,   -- .reserved
        0x00001b38,   -- .subsystem_base
        0x00001a19,   -- .etm_base
        0x00001a1a,   -- .l2cfg_base
        0x00001a1b,   -- .reserved2
        0x00001a80,   -- .l1s0_base
        0x00000000,   -- .axi2_lowaddr
        0x00001a1c,   -- .streamer_base
        0x00001a1d,   -- .clade_base
        0x00001a1e,   -- .fastl2vic_base
        0x00000080,   -- .jtlb_size_entries
        0x00000001,   -- .coproc_present
        0x00000004,   -- .ext_contexts
        0x00001a80,   -- .vtcm_base
        0x00002000,   -- .vtcm_size_kb
        0x00000400,   -- .l2tag_size
        0x00000400,   -- .l2ecomem_size
        0x0000003f,   -- .thread_enable_mask
        0x00001a1f,   -- .eccreg_base
        0x00000080,   -- .l2line_size
        0x00000000,   -- .tiny_core
        0x00000000,   -- .l2itcm_size
        0x00001a00,   -- .l2itcm_base
        0x00000000,   -- .clade2_base
        0x00000000,   -- .dtm_present
        0x00000001,   -- .dma_version
        0x00000007,   -- .hvx_vec_log_length
        0x00000000,   -- .core_id
        0x00000000,   -- .core_count
        0x00000040,   -- .hmx_int8_spatial
        0x00000020,   -- .hmx_int8_depth
        0x00000001,   -- .v2x_mode
        0x00000004,   -- .hmx_int8_rate
        0x00000020,   -- .hmx_fp16_spatial
        0x00000020,   -- .hmx_fp16_depth
        0x00000002,   -- .hmx_fp16_rate
        0x0000002e,   -- .hmx_fp16_acc_frac
        0x00000012,   -- .hmx_fp16_acc_int
        0x00000001,   -- .acd_preset
        0x00000001,   -- .mnd_preset
        0x00000010,   -- .l1d_size_kb
        0x00000020,   -- .l1i_size_kb
        0x00000002,   -- .l1d_write_policy
        0x00000040,   -- .vtcm_bank_width
        0x00000001,   -- .reserved3
        0x00000001,   -- .reserved4
        0x00000000,   -- .reserved5
        0x0000000a,   -- .hmx_cvt_mpy_size
        0x00000000,   -- .axi3_lowaddr
   };

local hexagon_cluster= {
    hexagon_num_threads = 1;
    hexagon_thread_0={start_powered_off = false};
    hexagon_thread_1={start_powered_off = true};
    hexagon_thread_2={start_powered_off = true};
    hexagon_thread_3={start_powered_off = true};
    HexagonQemuInstance = { tcg_mode="SINGLE", sync_policy = "multithread-unconstrained"};
    hexagon_start_addr = 0x8B500000;
    l2vic={  mem           = {address=0xfc910000, size=0x1000};
             fastmem       = {address=0xd81e0000, size=0x10000}};
    qtimer={ mem           = {address=0xfab20000, size=0x1000};
             mem_view      = {address=0xfc921000, size=0x2000};
            nr_frames=2;nr_views=1};
    pass = {target_socket   = {address=0x0       , size=0x40000000}};
    wdog = { socket        = {address=NSP0_AHBS_BASE + 0x84000, size=0x1000}};
    pll_0 = { socket        =  {address=NSP0_AHBS_BASE + 0x40000, size=0x10000}};
    pll_1 = { socket        =  {address=0x1b001000, size=0x10000}};
    pll_2 = { socket        =  {address=0x1b020000, size=0x10000}};
    pll_3 = { socket        =  {address=0x1b021000, size=0x10000}};
    rom=  {  target_socket = {address=CFGTABLE_BASE, size=0x100 },read_only=true, load={data=SA8540P_nsp0_config_table, offset=0}};
    csr = { socket = {address=0x1B300000, size=0x1000}};
};

platform = {
    arm_num_cpus = 8;
    num_redists  = 1;
    quantum_ns   = 100000000;
    with_gpu = true;

    ArmQemuInstance = { tcg_mode="MULTI", sync_policy = "multithread-unconstrained"};

    hexagon_ram={ target_socket         = {address=0x0000000000, size=0x0008000000}};
    ipcc=       { socket                = {address=0x0000410000, size=0x00000fc000}};
    uart=       { simple_target_socket_0= {address=0x0009000000, size=0x0000001000}, irq=1};
    virtioblk0= { mem                   = {address=0x000a003c00, size=0x0000002000}, irq=0x2e, blkdev_str = "file=" .. filesystem_image};
    virtionet0= { mem                   = {address=0x000a003e00, size=0x0000002000}, irq=0x2d, netdev_str = "type=user,hostfwd=tcp::2222-:22,hostfwd=tcp::2280-:80"};
    system_imem={ target_socket         = {address=0x0014680000, size=0x0000040000}};
 -- smmu=       { mem                   = {address=0x0015000000, size=0x0000100000};
    qtb=        { control_socket        = {address=0x0015180000, size=0x0000004000}}; -- + 0x4000*tbu number
    gic=        { dist_iface            = {address=0x0017100000, size=0x0000010000};
                  redist_iface_0        = {address=0x00171a0000, size=0x0000f60000}};
 -- gpex=       { pio_iface             = {address=0x003eff0000, size=0x0000010000};
    ram_0=      { target_socket         = {address=0x0040000000, size=0x0020000000}};
 -- gpex=       { mmio_iface            = {address=0x0060000000, size=0x002B500000};
    rom=        { target_socket         = {address=0x00de000000, size=0x0000000400},read_only=true};

    gpex=       { pio_iface             = {address=0x003eff0000, size=0x0000010000};
                  mmio_iface            = {address=0x0060000000, size=0x002B500000};
                  ecam_iface            = {address=0x4010000000, size=0x0010000000};
                  mmio_iface_high       = {address=0x8000000000, size=0x8000000000},
                  irq_0=3, irq_1=4, irq_2=5, irq_3=6};

    fallback_memory = { target_socket = { address=0x00000000, size=0x40000000},
                        dmi_allow = false,
                        verbose = true,
                        load = { csv_file=top().."fw/waipio/SM8450_Waipio.csv",
                                 offset=0,
                                 addr_str="Address",
                                 value_str="Reset Value",
                                 byte_swap=true
                               }
                      };

    hexagon_num_clusters = 1;
    hexagon_cluster_0 = hexagon_cluster;
    smmu = { mem = {address=0x15000000, size=0x100000};
            num_tbu=2;
            num_pages=128;
            num_cb=128;
            tbu_sid_0 = 0x1234;
            upstream_socket_0 = {address=0x0, size=0xd81e0000, relative_addresses=false};
            tbu_sid_1 = 0;
            upstream_socket_1 = {address=0x0, size=0x100000000, relative_addresses=false};
            irq_context = 103;
            irq_global = 65;
    };

        -- for virtio image
    load = {
        { bin_file=linux_image,
          address=_KERNEL64_LOAD_ADDR
        };

        { bin_file=device_tree,
          address=_DTB_LOAD_ADDR
        };

        { data=_bootloader_aarch64,
          address = 0x40000000};

--        { elf_file=top().."fw/hexagon-images/bootimage_kailua.cdsp.coreQ.pbn"};

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
        qtmr_rg0 = platform.hexagon_cluster_0.qtimer.mem_view.address,
        qtmr_rg1 = platform.hexagon_cluster_0.qtimer.mem_view.address + 0x1000,
    };
end
