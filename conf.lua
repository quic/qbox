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

local conf = {
    [ "platform.kernel_file" ] = top().."fw/linux-demo/Image",
    [ "platform.dtb_file" ] = top().."fw/linux-demo/platform.dtb",
    [ "platform.flash_blob_file" ] = top().."fw/linux-demo/rootfs.squashfs",
    [ "platform.hexagon.sync-policy" ] = "tlm2",
    [ "platform.hexagon.gdb-port" ] = 1234,
    [ "platform.hexagon_kernel_file" ] = top().."fw/linux-demo/helloworld.bin",
    [ "platform.hexagon_load_addr" ] = 0x0,
    [ "platform.hexagon_start_addr" ] = 0x60d8,

}

for k,v in pairs(conf) do
    _G[k]=v
end
