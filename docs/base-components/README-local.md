
[//]: # (SECTION 0)
## The GreenSocs SystemC simple components library.

This includes simple models such as routers, memories and exclusive monitor. The components are "Loosely timed" only. They support DMI where appropriate, and make use of CCI for configuration.

It also has several unit tests for memory, router and exclusive monitor.

[//]: # (SECTION 10)
## Information about building and using the base-components library
The base-components library depends on the libraries : Libgsutls, SystemC, RapidJSON, SystemCCI, Lua and GoogleTest.

[//]: # (SECTION 50)


## The GreenSocs component library loader
The loader components exposes a single initiator socket (`initiator_socket`) which should be bound to the bus fabric through which it's intended to load memory components.
The loader can be confgured to load the following:
 * elf files\
Configure parameter: `elf_file`\
function: `void elf_load(const std::string& path)`\
 _Note, no address or offset is possible, as they are built into the elf file. If this is specified within the context of a memory, the addresses will be offset within the memory, this is probably NOT what is desired._

 * bin_file\
  Configure parameter `bin_file`\
  options: `address` (absolute address), or `offset` (relative address)\
  function: `void file_load(std::string filename, uint64_t addr)`

 * csv_file (32 bit data)\
 Configure parameter `csv_file`\
 options: `address` (absolute address), or `offset` (relative address)\
        `addr_str` : header in the CSV file for the column used for addresses\
        `value_str`: header in the CSV file for the column used for values\
        `byte_swap`: whether bytes should be swapped\
  function: `void csv_load(std::string filename, uint64_t offset, std::string addr_str std::string value_str, bool byte_swap)`

* param\
Configurable parameter `param`\
A configuration paramter that must be of type `std::string` is loaded into memory. The parameter must be realized such that a typed handled can be obtained.\
options: `address` (absolute address), or `offset` (relative address)\
function: `void str_load(std::string data, uint64_t addr)`

* data (32 bit data)\
Configure parameter `data`\
options: `byte_swap`: whether bytes should be swapped\
The data parameter is expected to be an array with index's numbered. The indexes will be used to address the memory.\
No function access is provided.

* ptr\
No config parameter is provided.\
function `void ptr_load(uint8_t *data, uint64_t addr, uint64_t len)`

as an example, a configuration may look like
`loader = {{elf_file="my_elf.elf"}, {data={0x1,0x2}, address = 0x30}}`

## The GreenSocs component library memory
The memory component allows you to add memory when creating an object of type `Memory("name")`.
The memory should be bound with it's tarket socket:
`tlm_utils::simple_target_socket<Memory> socket`

The memory accepts the following configuration options:
`read_only`: Read Only Memory (default false)\
`dmi_allow`: DMI allowed (default true)\
`verbose`: Switch on verbose logging (default false)\
`latency`: Latency reported for DMI access (default 10 NS)\
`map_file`: file used to map this memory (such that it can be preserved through runs) (default,none)

The memory's size is set by the address space allocated to it on it's target socket.
An optional size can be provided to the constructor.
The size set by the configurator will take precedence. The size given to the constructor will take precedence over that set on the target socket at bind time.

The memory has a 'loader' built in. configuration can be applied to `memory_name.load`. In this case, all addresses are assumed to be offsets.

## Memory Dumper
A memory dumper is provided that can be used to debug the state of memory for debug purposes.
`outfile`: std::string file name that should be written. The binary file will be written to <memoryname>.start.end.<outfile name given>
`MemoryDumper_trigger`: bool trigger that when written to will trigger the dump to start.
The dumper must be bound to the main system router, it will find all memories in the system, find their addresses and request (via the initiator port) data from that memory.
A target port must also be bound, and the address to which it's bound, if accessed will trigger the dump.

## The GreenSocs component library router
The  router is a simple device, the expectation is that initiators and targets are directly bound to the router's `target_socket` and `initiator_socket` directly (both are multi-sockets).
The router will use CCI param's to discover the addresses and size of target devices as follows:
 * `<target_name>.<socket_name>.address`
 * `<target_name>.<socket_name>.size`
 * `<target_name>.<socket_name>.relative_addresses`

A default tlm2 "simple target socket" will have the name `simple_target_socket_0` by default (this can be changed in the target).
The `relative_addresses` flag is a boolean - targets which opt to have the router mask their address will receive addresses based from the IP base "address". Otherwise they will receive full addresses. The defaut is to receive relative addresses.

The router also offers `add_target(socket, base_address, size)` as a convenience, this will set appropriate param's (if they are not already set), and will set `relative_addresses` to be `true`.

Likewise the convenience function `add_initiator(socket)` allows multiple initiators to be connected to the router. Both `add_target` and `add_initiator` take care of binding.

__NB Routing is perfromed in _BIND_ order. In other words, overlapping addresses are allowed, and the first to match (in bind order) will be used. This allows 'fallback' routing.__

Also note that the router will add an extension called gs::PathIDExtension. This extension holds a std::vector of port index's (collectively a unique 'ID'). 

The ID is meant to be composed by all the routers on the path that
support this extension. This ID field can be used (for instance) to ascertain a unique ID for the issuing initiator.

The ID extension is held in a (thread safe) pool. Thread safety can be switched of in the code.

[//]: # (SECTION 100)
## The GreenSocs component Tests

It is possible to test the correct functioning of the different components with the tests that are proposed.

Once you have compiled your library, you will have a tests folder in your construction directory.

In this test folder you will find several folders, each corresponding to a component and each containing an executable.

You can run the executable with : 
```bash
./build/tests/<name_of_component>/<name_of_component>-tests
```

If you want a more general way to check the correct functioning of the components you can run all the tests at the same time with :
```bash
make test
```
