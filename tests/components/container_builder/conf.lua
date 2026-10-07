-- Configuration table for container1 with nested hierarchy
container1_config = {
    initiator1 = {
        moduletype = "InitiatorTester";
        initiator_socket = {bind = "&router.target_socket"};
    };

    router = {
        moduletype = "router";
    };

    memory_a = {
        moduletype = "gs_memory";
        target_socket = {
            address = 0x40000000;
            size = 0x1000;
            bind = "&router.initiator_socket";
        };
    };

    memory_b = {
        moduletype = "gs_memory";
        target_socket = {
            address = 0x50000000;
            size = 0x1000;
            bind = "&router.initiator_socket";
        };
    };

    nested_container = {
        moduletype = "Container";

        initiator_nested = {
            moduletype = "InitiatorTester";
            initiator_socket = {bind = "&router_nested.target_socket"};
        };

        router_nested = {
            moduletype = "router";
        };

        memory_nested = {
            moduletype = "gs_memory";
            target_socket = {
                address = 0x60000000;
                size = 0x1000;
                bind = "&router_nested.initiator_socket";
            };
        };
    };

    sockets = {
        external_initiator_to_router = "&router.target_socket";
        router_to_external = "&router.initiator_socket";
    };
};

-- Configuration table for container2
container2_config = {
    initiator2 = {
        moduletype = "InitiatorTester";
        initiator_socket = {bind = "&router.target_socket"};
    };

    router = {
        moduletype = "router";
    };

    memory_x = {
        moduletype = "gs_memory";
        target_socket = {
            address = 0x70000000;
            size = 0x1000;
            bind = "&router.initiator_socket";
        };
    };

    memory_y = {
        moduletype = "gs_memory";
        target_socket = {
            address = 0x80000000;
            size = 0x1000;
            bind = "&router.initiator_socket";
        };
    };

    sockets = {
        external_initiator_to_router = "&router.target_socket";
        router_to_external = "&router.initiator_socket";
    };
};

AllTests = {
    reuse_platform = {
        existing = {
            moduletype = "UnregisteredModule";
            dont_construct = true;
        };
        existing_container = {
            moduletype = "UnregisteredContainer";
            dont_construct = true;
        };
    };
    platform = {
        moduletype = "Container";

        initiator = {
            moduletype = "InitiatorTester";
            initiator_socket = {bind = "&router_main.target_socket"};
        };

        router_main = {
            moduletype = "router";
            target_socket = {
                address = 0x10000000;
                size = 0x20001000;
                relative_addresses = false;
            };
        };

        memory1 = {
            moduletype = "gs_memory";
            target_socket = {
                address = 0x10000000;
                size = 0x1000;
                bind = "&router_main.initiator_socket";
            };
        };

        memory2 = {
            moduletype = "gs_memory";
            target_socket = {
                address = 0x20000000;
                size = 0x1000;
                bind = "&router_main.initiator_socket";
            };
        };

        memory3 = {
            moduletype = "gs_memory";
            target_socket = {
                address = 0x30000000;
                size = 0x1000;
                bind = "&router_main.initiator_socket";
            };
        };

        container1 = {
            moduletype = "container_builder";
            config = container1_config;
            router_to_external = {
                bind = "&AllTests.platform.router_main.target_socket";
            };
        };

        container2 = {
            moduletype = "container_builder";
            config = container2_config;
        };
    };
}

for i = 1, 16 do
    local suffix = string.format("%02d", i)
    AllTests.platform["a_router_order_probe_" .. suffix] = {
        moduletype = "router_order_probe";
        initiator_socket = {bind = "&z_order_memory_" .. suffix .. ".target_socket"};
    }
    AllTests.platform["z_order_memory_" .. suffix] = {
        moduletype = "gs_memory";
        target_socket = {
            address = 0xC0000000 + i * 0x1000;
            size = 0x1000;
        };
    }
end

AllTests.platform.a_nested_router_order_probe = {
    moduletype = "nested_router_order_probe";
    ports = {
        [0] = {bind = "&z_nested_router_order_memory.target_socket"};
    };
};
AllTests.platform.z_nested_router_order_memory = {
    moduletype = "gs_memory";
    target_socket = {
        address = 0xD1000000;
        size = 0x1000;
        bind = "&router_main.initiator_socket";
    };
};
