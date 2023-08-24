print("  local configuration in " .. top());

local NUM_GPUS = 1

tableMerge(platform, {

    virtioblk_0 = {
        moduletype="QemuVirtioMMIOBlk",
        mem = { address = 0x1c0d0000, size = 0x2000, bind = "&router.initiator_socket"},
        irq_out = {bind = "&gic_0.spi_in_9"},
        blkdev_str = "file=" .. valid_file(top() .. "system_lemans_qdrive_qvp.img") .. ",format=raw,if=none,readonly=off" };
});

tableJoin(platform["load"], {
    { bin_file = valid_file(top() .. "bl31_lem.bin"), address = INITIAL_DDR_SPACE_14GB };
    { bin_file = valid_file(top() .. "mifs_qdrive_qvp.img"), address = INITIAL_DDR_SPACE_14GB + OFFSET_MIFS_DDR_SPACE };

    { bin_file = valid_file(top() .. "smem_init.bin"), address = INITIAL_DDR_SPACE_14GB + OFFSET_SMEM_DDR_SPACE };

    { bin_file = valid_file(top() .. "cmd_db_header.bin"), address = 0x0C3F0000 };
    { bin_file = valid_file(top() .. "cmd_db.bin"), address = 0x80860000 };
    { elf_file = valid_file(top() .. "/../../dsp/bootimage_lemans.cdsp0.prodQ.pbn") };
    { elf_file = valid_file(top() .. "/../../dsp/bootimage_lemans.cdsp1.prodQ.pbn") };
});

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
