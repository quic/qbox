print("  local configuration in " .. top());

-- This allows to specify a different directory for the binaries
local LRH_IMAGES_DIR
if os.getenv("LRH_IMAGES_DIR") == nil then
    LRH_IMAGES_DIR = top()
else
    LRH_IMAGES_DIR = os.getenv("LRH_IMAGES_DIR") .."/"
end


--Initialize Kernel and DTB Address
_KERNEL64_LOAD_ADDR = 0xB0000000 + 0x00080000
_DTB_LOAD_ADDR = 0xba8fe000

dofile(top() .. "/../../../../arm64_bootloader.lua")

--Overwrite UART Address
local PCIE_3APCIE_WRAPPER_AXI_G3X4_EDMA_AUTO = 0x40000000
local UART0 = PCIE_3APCIE_WRAPPER_AXI_G3X4_EDMA_AUTO

-- QUPv3 uart
local QUPV3_1_SE2 = 0x00A8C000

--Overwrite & disable QUPV3_1
platform["uart_qup_17"].input = false

tableMerge(platform, {
    virtioblk_0 = { mem = { address = 0x160d0000, size = 0x2000 }, irq = 0x2e, blkdev_str = "file=" .. valid_file(LRH_IMAGES_DIR .. "rootfs.ext4") .. ",format=raw,if=none,readonly=off" };
    virtioblk_1 = { mem = { address = 0x160d2000, size = 0x2000 }, irq = 0x2f, blkdev_str = "file=" .. valid_file(LRH_IMAGES_DIR .. "disk20G.img") .. ",format=raw,if=none,readonly=off" };
    -- TODO: check for overlaps with ram_0 and ram_1
    ram_2 = { target_socket = { address = 0xd00000000, size = 0x280000000 } };
    ram_3 = { target_socket = { address = 0xA80000000, size = 0x180000000 } };
    ram_4 = { target_socket = { address = 0x940000000, size = 0x140000000 } };
    ram_5 = { target_socket = { address = 0x100000000, size = 0x300000000 } };
    ram_6 = { target_socket = { address = 0x900000000, size = 0x32400000 } };

    -- Overwrite uart irq to match dtb & newly added parameters to get input for id/password
    uart= {  simple_target_socket_0 = {address=UART0, size=0x1000},
        irq=0x17B,
        stdio=true,
        input=true,
        port=nil,
    };
-- Overwrite virtionet irq
    virtionet0= { mem    =   {address=0x16120000, size=0x10000}, irq=0x2d, netdev_str="type=user,hostfwd=tcp::2222-:22,hostfwd=tcp::2221-:21,hostfwd=tcp::56283-:56283,hostfwd=tcp::55534-:65534,hostfwd=tcp::55535-:65535"};
});

for i=0,(platform.arm_num_cpus-1) do
  local cpu = platform["cpu_"..tostring(i)]
  cpu.has_el2 = true;
  cpu.psci_conduit = "disabled";
  cpu.start_powered_off = false;
  cpu.rvbar = INITIAL_DDR_SPACE_14GB;
end

tableJoin(platform["load"], {
  {data={0x80000000},  address=0x3d96004};
  { bin_file = valid_file(LRH_IMAGES_DIR .. "bl31_rh.bin"), address = INITIAL_DDR_SPACE_14GB };
  { elf_file = valid_file(LRH_IMAGES_DIR .. "hypvm_with_sim.elf") };
  { data = _bootloader_aarch64, address = 0xb0000000 };
  { bin_file = valid_file(LRH_IMAGES_DIR .. "vmlinux"), address = _KERNEL64_LOAD_ADDR };
  { bin_file = valid_file(LRH_IMAGES_DIR .. "lemans_vp.dtb"), address = _DTB_LOAD_ADDR };
  { bin_file = valid_file(LRH_IMAGES_DIR .. "smem_abl.bin"), address = INITIAL_DDR_SPACE_14GB + OFFSET_SMEM_DDR_SPACE };
  { bin_file = valid_file(LRH_IMAGES_DIR .. "cmd_db_header.bin"), address = 0x0C3F0000 };
  { bin_file = valid_file(LRH_IMAGES_DIR .. "cmd_db.bin"), address = 0x80860000 };
});

platform.gpex.irq_0=3
platform.gpex.irq_1=4
platform.gpex.irq_2=5
platform.gpex.irq_3=6

print (_KERNEL64_LOAD_ADDR);
print (_DTB_LOAD_ADDR);
platform["with_gpu"] = true;
