print("  local configuration in " .. top());

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
    virtioblk_0 = { mem = { address = 0x160d0000, size = 0x2000 }, irq = 0x2e, blkdev_str = "file=" .. valid_file(top() .. "rootfs.ext4") .. ",format=raw,if=none,readonly=off" };
--    ram_2 = { target_socket = { address = 0xd00000000, size = 0x252c00000 } };
--    ram_3 = { target_socket = { address = 0x940000000, size = 0x292d00000 } };
--    ram_4 = { target_socket = { address = 0x900000000, size = 0x35200000 } };

-- Overwrite uart irq to match dtb & newly added parameters to get input for id/password
	uart = { simple_target_socket_0 = {address= UART0, size=0x1000},
        irq=0x17B,
        stdio=true,
        input=true,
        port=nil,
    };
-- Overwrite virtionet irq
    virtionet0= { mem    =   {address=0x16120000, size=0x10000}, irq=0x2d, netdev_str="type=user,hostfwd=tcp::2222-:22,hostfwd=tcp::2221-:21,hostfwd=tcp::56283-:56283,hostfwd=tcp::55534-:65534,hostfwd=tcp::55535-:65535"};
});

tableJoin(platform["load"], {

{ bin_file = valid_file(top() .. "bl31_rh.bin"), address = INITIAL_DDR_SPACE_14GB };
{ bin_file = valid_file(top() .. "vmlinux"), address = _KERNEL64_LOAD_ADDR };
{ bin_file = valid_file(top() .. "lemans_vp.dtb"), address = _DTB_LOAD_ADDR };
});

print (_KERNEL64_LOAD_ADDR);
print (_DTB_LOAD_ADDR);
