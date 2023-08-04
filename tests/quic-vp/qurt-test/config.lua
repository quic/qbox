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

-- Change CDSP0's boot image
for _, load in ipairs(platform.load) do
  if load.elf_file ~= nil and string.find(load.elf_file, "cdsp0")
  then
    -- Note: we expect this to be at the same directory of the config!
    load.elf_file=top().."bootimage.pbn"
  end
end

-- Entry point from the QURT bootimage files
platform.hexagon_cluster_0.hexagon_start_addr=0x50000000
local id = 0
while true do
  ram = "ram_"..id
  if platform[ram] == nil
  then
    platform[ram] = { target_socket = {address=0x50000000, size=100*1024*1024}; }
    break
  end
  id = id + 1
end

-- We need the vp to exit when all hexagon threads finish
platform.hexagon_cluster_0.vp_mode=false

-- Disable other CDSPs and filter out non CDSP0 IRQs
platform.hexagon_num_clusters=1
local new_irqs = {}
for _, irq in ipairs(platform.ipcc.irqs) do
  if string.find(irq.dst, "hexagon_cluster_0")
  then
    table.insert(new_irqs, irq)
  end
end
platform.ipcc.irqs = new_irqs

platform.with_gpu=false
