print("  local configuration in " .. top());

-- This allows to specify a different directory for the binaries
local LRH_IMAGES_DIR
if os.getenv("LRH_IMAGES_DIR") == nil then
    LRH_IMAGES_DIR = top()
else
    LRH_IMAGES_DIR = os.getenv("LRH_IMAGES_DIR") .."/"
end


--Initialize Kernel and DTB Address
_KERNEL64_LOAD_ADDR = INITIAL_DDR_SPACE_14GB + 0x00080000
_DTB_LOAD_ADDR = INITIAL_DDR_SPACE_14GB + 0x03400000

dofile(top() .. "/../../../../arm64_bootloader.lua")

--Overwrite UART Address
local PCIE_3APCIE_WRAPPER_AXI_G3X4_EDMA_AUTO = 0x40000000
local UART0 = PCIE_3APCIE_WRAPPER_AXI_G3X4_EDMA_AUTO

--Overwrite & disable QUPV3_1
platform["uart_qup_17"].input = false

-- Number of gpus
local NUM_GPUS = 1,

tableMerge(platform, {
    virtioblk_0 = {
        moduletype="QemuVirtioMMIOBlk",
        args = {"&platform.qemu_inst"};
        mem = { address = 0x160d0000, size = 0x2000, bind = "&router.initiator_socket"},
        irq_out = {bind = "&gic_0.spi_in_46"},
        blkdev_str = "file=" .. valid_file(LRH_IMAGES_DIR .. "rootfs.ext4") .. ",format=raw,if=none,readonly=off" };

    -- Overwrite uart irq to match dtb & newly added parameters to get input for id/password
   charbackend_stdio_0 = {
       moduletype = "CharBackendStdio";
       args = {true};
   };

   pl011 = {
       moduletype = "Pl011",
       args = {"&platform.charbackend_stdio_0"};
       target_socket = {address= UART0,
                                 size=0x1000, 
                                 bind = "&router.initiator_socket"},
       irq = {bind = "&gic_0.spi_in_379"},
   };

-- Overwrite virtionet irq
    virtionet0= {
        moduletype = "QemuVirtioMMIONet",
        args = {"&platform.qemu_inst"};
        mem    =   {address=0x16120000, size=0x10000, bind = "&router.initiator_socket"},
        irq_out={bind = "&gic_0.spi_in_45"},
        netdev_str="tap,ifname=tap0,script=" .. top() .. "/adb_debug,downscript=no"};

    gpex_0 =       {
        moduletype = "QemuGPEX";
        args = {"&platform.qemu_inst"};
        bus_master = {"&router.target_socket"};
        pio_iface             = {address=0x60200000, size=0x0000100000, bind= "&router.initiator_socket"};
        mmio_iface            = {address=0x60300000, size=0x001fd00000, bind= "&router.initiator_socket"};
        ecam_iface            = {address=0x43B50000, size=0x0010000000, bind= "&router.initiator_socket"};
        mmio_iface_high       = {address=0x9000000000, size=0x8000000000, bind= "&router.initiator_socket"},
        irq_out_0 = {bind = "&gic_0.spi_in_3"};
        irq_out_1 = {bind = "&gic_0.spi_in_4"};
        irq_out_2 = {bind = "&gic_0.spi_in_5"};
        irq_out_3 = {bind = "&gic_0.spi_in_6"};
        irq_num_0 = 0;
        irq_num_1 = 0;
        irq_num_2 = 0;
        irq_num_3 = 0;
    };

});

tableJoin(platform["load"], {
  { bin_file = valid_file(LRH_IMAGES_DIR .. "bl31_rh.bin"), address = INITIAL_DDR_SPACE_14GB };
  { bin_file = valid_file(LRH_IMAGES_DIR .. "vmlinux"), address = _KERNEL64_LOAD_ADDR };
  { bin_file = valid_file(LRH_IMAGES_DIR .. "lemans_vp.dtb"), address = _DTB_LOAD_ADDR };
});

-- tableMerge(platform, {
--     gpu_0 = {
--         moduletype="QemuVirtioGpuGlPci",
--         args = {"&platform.qemu_inst"};
--     };

--     gpex_0 =       {
--         moduletype = "QemuGPEX";
--         args = {"&platform.qemu_inst"};
--         bus_master = {"&router.target_socket"};
--         pio_iface             = {address=0x60200000, size=0x0000100000, bind= "&router.initiator_socket"};
--         mmio_iface            = {address=0x60300000, size=0x001fd00000, bind= "&router.initiator_socket"};
--         ecam_iface            = {address=0x43B50000, size=0x0010000000, bind= "&router.initiator_socket"};
--         mmio_iface_high       = {address=0x9000000000, size=0x8000000000, bind= "&router.initiator_socket"},
--         irq_out_0 = {bind = "&gic_0.spi_in_3"};
--         irq_out_1 = {bind = "&gic_0.spi_in_4"};
--         irq_out_2 = {bind = "&gic_0.spi_in_5"};
--         irq_out_3 = {bind = "&gic_0.spi_in_6"};
--         irq_num_0 = 0;
--         irq_num_1 = 0;
--         irq_num_2 = 0;
--         irq_num_3 = 0;
--     };
-- });

print (_KERNEL64_LOAD_ADDR);
print (_DTB_LOAD_ADDR);
platform["with_gpu"] = true;

if (platform.with_gpu == true) then
    if (NUM_GPUS > 0) then
        for i=0,(NUM_GPUS-1) do
            print (i);
            if (i==0) then
                gpu = {
                    moduletype = "QemuVirtioGpuGlPci";
                    args = {"&platform.qemu_inst"};
                }
            else
                gpu = {
                    moduletype = "QemuVirtioGpuPci";
                    args = {"&platform.qemu_inst"};
                }
            end
            platform["gpu_"..tostring(i)]=gpu;
        end

        display = {
            moduletype = "QemuDisplay";
            args = {"&platform.gpu_0"};
        }
        platform["display_0"]=display;
    end
end
