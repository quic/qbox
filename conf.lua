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

linux_image = get_image ("bsp/linux/out/android-mainline/common/arch/arm64/boot/Image",
                         image_install_dir().."Image",
                         "Image");

device_tree = get_image ("bsp/linux/extras/dts/vp.dtb",
                         image_install_dir().."vp.dtb",
                         "vp.dtb");

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
             timer0_mem    = {address=0xfc921000, size=0x1000};
             timer1_mem    = {address=0xfc922000, size=0x1000}};
    pass = {target_socket   = {address=0x0       , size=0x40000000}};
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
                        load = { csv_file=top().."fw/SM8450_Waipio.csv",
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
        qtmr_rg0 = platform.hexagon_cluster_0.qtimer.timer0_mem.address,
        qtmr_rg1 = platform.hexagon_cluster_0.qtimer.timer1_mem.address,
    };
end
