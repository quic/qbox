-- Virtual platform configuration
-- Commented out parameters show default values

function top()
    local str = debug.getinfo(2, "S").source:sub(2)
    if str:match("(.*/)")
    then
        return str:match("(.*/)")
    else
        return "./"
    end
 end

num_arm = 8
num_hex = 8

local conf = {
    [ "platform.arm-num-cpus" ] = num_arm,
    [ "platform.hexagon-num-cpus" ] = num_hex,
    [ "platform.quantum-ns"] = 100000000,
    [ "platform.kernel_file" ] = top().."fw/fastrpc-images/images/Image",
    [ "platform.dtb_file" ] = top().."fw/fastrpc-images/images/rumi.dtb",
--    [ "platform.bootloader_file" ] = top().."fw/fastrpc-images/images/hypvm.elf",
    [ "platform.vendor_flash_blob_file" ] = top().."fw/fastrpc-images/images/vendor.squashfs",
    [ "platform.system_flash_blob_file" ] = top().."fw/fastrpc-images/images/system.squashfs",

--    [ "platform.cpu_0.gdb-port" ] = 1234,
--    [ "platform.hexagon.gdb-port" ] = 1234,

-- [ "platform.hexagon_kernel_file" ] = top().."tests/qualcomm/prebuilt/qtimer_test.bin",
    [ "platform.hexagon_kernel_file" ] = top().."fw/hexagon-images/bootimage_kailua.cdsp.coreQ.pbn",
    [ "platform.hexagon_load_addr" ] = 0x0,
--[ "platform.hexagon_start_addr" ] = 0x50000000, // for qtimer_test
    [ "platform.hexagon_start_addr" ] = 0x8B500000,
    [ "platform.hexagon_isdb_secure_flag" ] = 0x1,
    [ "platform.hexagon_isdb_trusted_flag" ] = 0x1,
}

for i=0,num_arm do
    conf["platform.cpu_"..tostring(i)..".sync-policy"] = "multithread-unconstrained"
end
for i=0,num_hex do
    conf["platform.hexagon-cpu_"..tostring(i)..".sync-policy"] = "multithread-unconstrained"
end


for k,v in pairs(conf) do
    _G[k]=v
end
