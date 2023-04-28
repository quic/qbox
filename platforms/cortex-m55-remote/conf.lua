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

platform = {
    ram = {target_socket = {address=0x0, size=0x20000}, shared_memory=true, 
            load={bin_file=top().."fw/cortex-m55/cortex-m55.bin", offset=0}},
    uart =  { simple_target_socket_0 = {address= 0xc0000000, size=0x1000}, irq=0},

    remote = {
        remote_exec_path=top().."../../build/platforms/cortex-m55-remote/remote_cpu",
        -- remote_argv = {
        --    {key = "random_arg1", val = "value of random_arg1"},
        --    {key = "random_arg1", val = "value of random_arg2"}}
    }
}