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
    moduletype = "Container";
    quantum_ns = 10000000;


    router = {
        moduletype="Router";
        log_level=0;
    },
    ram_0 = {
            moduletype = "Memory",
            target_socket = {address=0x0, size=0x20000, bind = "&router.initiator_socket"},
            shared_memory=true,
            load={bin_file=top().."fw/cortex-m55/cortex-m55.bin", offset=0},
        },
    
    charbackend_stdio_0 = {
       moduletype = "CharBackendStdio";
       args = {true};
        };

    uart_0 =  {
        moduletype = "Pl011",
        args = {"&platform.charbackend_stdio_0"},
        target_socket = {address= 0xc0000000, size=0x1000, bind = "&router.initiator_socket"},
        irq = {bind = "&local_pass_0.target_signal_socket_0"},
        },

   
    local_pass_0 = {
        moduletype = "PassRPC",
        args = {top().."../../build/platforms/cortex-m55-remote/remote_cpu"},
        tlm_initiator_ports_num = 1,
        tlm_target_ports_num = 0,
        target_signals_num = 1,
        initiator_socket_0 = {bind = "&router.target_socket"},
        remote_argv = {"--param=log_level=2"}, -- remote_argv = {"foo", "bar", "zee"};
    },
    
}