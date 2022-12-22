
function top()
    local str = debug.getinfo(2, "S").source:sub(2)
    if str:match("(.*/)")
    then
        return str:match("(.*/)")
    else
        return "./"
    end
 end

print ("Lua config running. . .");

test_bench = {
    rom1=   { target_socket  = {address=0x0000, size=0x1000}};
    rom2=   { target_socket  = {address=0x1000, size=0x1000}};
    rom3=   { target_socket  = {address=0x2000, size=0x1000}};

    load={
        {bin_file=top().."/src/loader-test.bin",    address=0x1000};
        {elf_file=top().."/src/loader-test.elf"};
        {csv_file=top().."/src/loader-test.csv",
            addr_str="_addr", value_str=" _value",
            offset = 0x100, address = 0x2000};
        {data={0xdeadbeaf}, address = 0x0500};
    }
};
SimpleReadData = test_bench;
SimpleReadELFFile = test_bench;
SimpleReadBinFile = test_bench;
SimpleReadCSVFile = test_bench;
