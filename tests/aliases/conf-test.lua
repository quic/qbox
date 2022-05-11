
print ("Lua config running. . .");

test_bench = {
    target= { target_socket = {address=0x0000, size=0xFF, aliases={{address=0X0100, size=0xFF}, {address=0x0200, size=0xFF}}}};
    ram=   { target_socket  = {address=0x0300, size=0xFF}};
    rom=   { target_socket = {address=0x0400, size=0xFF, aliases={{address=0X0500, size=0xFF}, {address=0x0600, size=0xFF}}}};
};