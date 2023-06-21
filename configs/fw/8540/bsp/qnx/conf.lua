print("  local configuration in " .. top());
tableMerge(platform, {
    virtioblk_0 = { mem = { address = 0x1c0d0000, size = 0x2000 }, irq = 9, blkdev_str = "file=" .. valid_file(top() .. "system_8540_qdrive_qvp.img") .. ",format=raw,if=none,readonly=off" };
    ram_1 = { target_socket = { address = 0x800000000, size = 0x80000000 } };
    ram_2 = { target_socket = { address = 0xc0000000, size = 0x340000000 } };
});

tableJoin(platform["load"], {
    { bin_file = valid_file(top() .. "bl31.bin"), address = INITIAL_DDR_SPACE_14GB };
    { bin_file = valid_file(top() .. "mifs_qdrive_qvp.img"), address = INITIAL_DDR_SPACE_14GB + OFFSET_MIFS_DDR_SPACE };
    { bin_file = valid_file(top() .. "smem_init.bin"), address = INITIAL_DDR_SPACE_14GB + OFFSET_SMEM_DDR_SPACE };
    { bin_file = valid_file(top() .. "cmd_db_header.bin"), address = 0x0C3F0000 };
    { bin_file = valid_file(top() .. "cmd_db.bin"), address = 0x80860000 };
    { elf_file = valid_file(top() .. "/../../dsp/bootimage_relocflag_withdummyseg_makena.cdsp0.prodQ.pbn") };
    { elf_file = valid_file(top() .. "/../../dsp/bootimage_relocflag_withdummyseg_makena.cdsp1.prodQ.pbn") };
});

platform["with_gpu"] = true;
