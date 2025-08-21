print ("Lua config running. . .");

test_bench = {
    target= { target_socket = {address=0x0000, size=0x1000, aliases={
        {address=0x1000, size=0x1000}, -- Alias for m_memory at 0x1000
        {address=0x4000, size=0x1000}  -- Alias for m_memory at 0x4000
    }}};
    ram=   { target_socket  = {address=0x5000, size=0x1000}}; -- m_ram moved to 0x5000
    rom=   { target_socket = {address=0x2000, size=0x1000, aliases={{address=0x3000, size=0x1000}}}};
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
