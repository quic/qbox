
print ("Lua config running. . .");

test_bench = {
    target= { target_socket = {address=0x0000, size=0xFF, aliases={{address=0X0100, size=0xFF}, {address=0x0200, size=0xFF}}}};
    ram=   { target_socket  = {address=0x0300, size=0xFF}};
    rom=   { target_socket = {address=0x0400, size=0xFF, aliases={{address=0X0500, size=0xFF}, {address=0x0600, size=0xFF}}}};
};
SimpleWriteRead = test_bench;
SimpleOverlapWrite = test_bench;
SimpleCrossesBoundary = test_bench;
SimpleWriteReadDebug = test_bench;
SimpleOverlapWriteDebug = test_bench;
SimpleCrossesBoundaryDebug = test_bench;
WriteBlockingReadDebug = test_bench;
WriteDebugReadBlocking = test_bench;
SimpleDmi = test_bench;
DmiWriteRead = test_bench;