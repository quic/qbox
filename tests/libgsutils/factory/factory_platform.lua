platform = {
    moduletype="Container";

    initiator = {
        moduletype="InitiatorTester";
        initiator_socket= {bind = "&router.target_socket";}
    },

    router = {
        moduletype="router";
    },

    rom_0 = {
        moduletype="memory";
        target_socket= {size = 0x1000,
                        address=0x0000;
                        bind = "&router.initiator_socket";}
    },

    rom_1 = {
        moduletype="memory";
        target_socket= {size = 0x1000,
                        address=0x1000;
                        bind = "&router.initiator_socket";}
    },

}