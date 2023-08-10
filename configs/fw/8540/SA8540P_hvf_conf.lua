-- Virtual platform configuration with HVF acceleration

local function top()
    local str = debug.getinfo(2, "S").source:sub(2)
    if str:match("(.*/)")
    then
        return str:match("(.*/)")
    else
        return "./"
    end
end
dofile(top() .. "../utils.lua");

ACCEL="hvf"
dofile(valid_file(top() .. "SA8540P_conf.lua"))
