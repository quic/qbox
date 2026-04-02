print("--- generic_lua_model testbench conf.lua ---")
Tester = {
    log_level = 5;
    
    memory = {
        target_socket = {address = 0x80000000, size = 0x380000000};
    };

    reg_memory = {
        target_socket = {address = 0x0, size = 0x80000000};
    };

    reg_router = {
        target_socket = {address = 0x0, size = 0x80000000};
    };

    gen_lua_model = {
        target_socket = {address = 0xb80000, size = 0x1000};

        -- These will be triggered after after_ns from the start of simulation 
        {after_ns = 200, address = 0x8000000C, data = 0xabcd1234};

        {after_ns = 200, address = 0x80000010, data = 0x56784321};

        {after_ns = 400, address = 0x80000020, data = 0x1234abcd};

        {after_ns = 400, address = 0x8000002C, data = 0x43215678};

        {after_ns = 400, name = "output_sig0", output = false};

        -- registers definition
        registers = {
            {
                name = "reg0";
                address = 0x0;
                reg_mask = 0xf000000f;
                default = 0xffffffff;
                on_pre_write = {
                    {
                        value = 0x0;
                        mask = 0x0ffffff0;
                    };
                };
                on_post_write = {
                    {
                        address = 0x80000200;
                        data = 0xababcdcd;
                    };
                };
            };
            {
                name = "reg1";
                address = 0x4;
                on_post_read = {
                    {
                        address = 0x80000004;
                        data = 0xbeefdead;
                        after_ns = 20;
                    };
                    {
                        address = 0x80000008;
                        data = 0x1234abcd;
                        after_ns = 40;
                    };
                    {
                        name = "output_sig1";
                        output = true;
                    };
                };
            };
            {
                name = "reg2";
                address = 0x8;
                on_post_write = {
                    {
                        address = 0x80000600;
                        data = 0x24246868;
                        if_val = 0x12345678;
                        after_ns = 20;
                    };
                };
                on_pre_read = {
                    {
                        value = 0xf1f1abab;
                        mask = 0x0ffffff0;
                        if_val = 0x12345678;
                    }
                };
            };
        };

        -- signals definition
        signals = {
            {
                name = "target_sig_0";
                on_true = {
                    {
                        address = 0x80000000;
                        data = 0xdeadbeef;
                        after_ns = 1;
                    };
                    {
                        name = "output_sig1";
                        output = true;
                    };
                };
                on_false = {
                    {
                        address = 0x80000000;
                        data = 0xbeefdead;
                    };
                    {
                        name = "output_sig1";
                        output = false;
                    };
                };
            };
            {
                name = "target_sig_1";
                on_true = {
                    {
                        address = 0x80000400;
                        data = 0xdeadbeef;
                    };
                };
            };
        };

    };

};
