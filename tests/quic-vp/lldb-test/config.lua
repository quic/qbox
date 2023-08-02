-- Virtual platform configuration

function top()
    local str = debug.getinfo(2, "S").source:sub(2)
    if str:match("(.*/)")
    then
        return str:match("(.*/)")
    else
        return "./"
    end
 end

dofile(top().."../../../configs/fw/8775/SA8775P_conf.lua");

for _, load in ipairs(platform.load) do
  if load.elf_file ~= nil and string.find(load.elf_file, "cdsp0")
  then
    load.elf_file = top().."cdsp0_image"
  end
end

platform.hex_plugin.hexagon_cluster_0.hexagon_start_addr=0x0
platform.hex_plugin.hexagon_cluster_0.hexagon_thread_0.gdb_port=1234
platform.with_gpu=false
