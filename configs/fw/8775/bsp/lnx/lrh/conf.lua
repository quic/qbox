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

tableMerge(platform, {
    virtioblk_0 = { mem = { address = 0x160d0000, size = 0x2000 }, irq = 0x2e, blkdev_str = "file=" .. valid_file(LRH_IMAGES_DIR .. "rootfs.ext4") .. ",format=raw,if=none,readonly=off" };

    -- Overwrite uart irq to match dtb & newly added parameters to get input for id/password
    uart= {  simple_target_socket_0 = {address=UART0, size=0x1000},
        irq=0x17B,
        stdio=true,
        input=true,
        port=nil,
    };
-- Overwrite virtionet irq
    virtionet0= { mem    =   {address=0x16120000, size=0x10000}, irq=0x2d, netdev_str="tap,ifname=tap0,script=" .. top() .. "/adb_debug,downscript=no"};
});

tableJoin(platform["load"], {
  { bin_file = valid_file(LRH_IMAGES_DIR .. "bl31_rh.bin"), address = INITIAL_DDR_SPACE_14GB };
  { bin_file = valid_file(LRH_IMAGES_DIR .. "vmlinux"), address = _KERNEL64_LOAD_ADDR };
  { bin_file = valid_file(LRH_IMAGES_DIR .. "lemans_vp.dtb"), address = _DTB_LOAD_ADDR };
});

tableMerge(platform, {
    gpu_0 = 1;
    gpex=       { pio_iface             = {address=0x60200000, size=0x0000100000};
    mmio_iface            = {address=0x60300000, size=0x001fd00000};
    ecam_iface            = {address=0x43B50000, size=0x0010000000};
    mmio_iface_high       = {address=0x9000000000, size=0x8000000000},
    irq_0=3, irq_1=4, irq_2=5, irq_3=6};
});

print (_KERNEL64_LOAD_ADDR);
print (_DTB_LOAD_ADDR);
platform["with_gpu"] = true;
