print("  local configuration in " .. top());
tableMerge(platform, {
    virtioblk_0 = { mem = { address = 0x1c0d0000, size = 0x2000 }, irq = 9,
        blkdev_str = "file=" .. valid_file(top() .. "system_qdrive_qvp.img") .. ",format=raw,if=none,readonly=on" };
    ram_2 = { target_socket = { address = 0x800000000, size = 0x80000000 } };
    ram_3 = { target_socket = { address = 0xc0000000, size = 0x340000000 } };
});

tableJoin(platform["load"], {
    { elf_file = valid_file(top() .. "bl31_CRM48.elf") };
    { bin_file = valid_file(top() .. "mifs_qdrive_qvp.img"),
        address = INITIAL_DDR_SPACE_14GB + OFFSET_MIFS_DDR_SPACE };
    { bin_file = valid_file(top() .. "smem_early_qdrive_16gb.bin"),
        address = INITIAL_DDR_SPACE_14GB + OFFSET_SMEM_DDR_SPACE };
});

platform["with_gpu"] = true;

