-- Virtual platform configuration
-- Commented out parameters show default values
local conf = {
    [ "platform.kernel_file" ] = "fw/linux-demo/Image",
    [ "platform.dtb_file" ] = "fw/linux-demo/platform.dtb",
    [ "platform.flash_blob_file" ] = "fw/linux-demo/rootfs.squashfs",
}

for k,v in pairs(conf) do
    _G[k]=v
end
