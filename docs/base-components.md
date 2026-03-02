# Base Components

The base-components library provides simple SystemC models
including routers, memories, loaders, and an exclusive monitor.
All components are "loosely timed" only, support DMI where
appropriate, and use CCI for configuration.

## Loader

The loader exposes a single initiator socket
(`initiator_socket`) which should be bound to the bus fabric
through which it loads memory contents. It supports the
following load types:

### ELF Files

- CCI parameter: `elf_file`
- Function: `void elf_load(const std::string& path)`
- Addresses are built into the ELF file; no address or offset
  parameter is available. If used within a memory context,
  addresses are offset within the memory -- this is probably
  not the desired behavior.

### Binary Files

- CCI parameter: `bin_file`
- Options: `address` (absolute) or `offset` (relative)
- Function:
  `void file_load(std::string filename, uint64_t addr, uint64_t file_offset = 0, uint64_t file_data_len = max)`

### CSV Files (32-bit data)

- CCI parameter: `csv_file`
- Options: `address` (absolute) or `offset` (relative),
  `addr_str` (CSV column header for addresses), `value_str`
  (CSV column header for values), `byte_swap`
- Function:
  `void csv_load(std::string filename, uint64_t offset, std::string addr_str, std::string value_str, bool byte_swap)`

### ZIP Archives

- CCI parameter: `zip_archive`
- Options: `address` (absolute) or `offset` (relative),
  `archived_file_name`, `archived_file_offset`,
  `archived_file_size`, `is_compressed`
- Function:
  `void zip_file_load(zip_t* p_archive, const std::string& archive_name, uint64_t addr, ...)`

### String Parameter

- CCI parameter: `param`
- A `std::string` configuration parameter is loaded into
  memory. The parameter must be realized such that a typed
  handle can be obtained.
- Options: `address` (absolute) or `offset` (relative)
- Function: `void str_load(std::string data, uint64_t addr)`

### Data Array (32-bit data)

- CCI parameter: `data`
- Options: `byte_swap`
- The data parameter is an array with numbered indices used
  as memory addresses.

### Raw Pointer

- No CCI parameter. Programmatic access only.
- Function: `void ptr_load(uint8_t *data, uint64_t addr, uint64_t len)`

### Example Configuration

```lua
loader = {
    {elf_file = "my_elf.elf"},
    {data = {0x1, 0x2}, address = 0x30}
}
```

## Memory

The memory component provides a simple target socket named
`target_socket`.

### CCI Parameters

| Parameter | Default | Description |
|-----------|---------|-------------|
| `read_only` | `false` | Read-only memory |
| `dmi_allow` | `true` | Allow DMI access |
| `latency` | 10 NS | Latency reported for DMI access |
| `map_file` | none | File used to map this memory (persists across runs) |

### Size

The memory size is determined by (in order of precedence):
1. The size set by the configurator
2. The size given to the constructor
3. The size set on the target socket at bind time

### Built-in Loader

Each memory has a built-in loader. Configuration can be
applied to `memory_name.load`. In this case, all addresses
are treated as offsets.

## Memory Dumper

A debugging utility for dumping memory state to files.

- `outfile` (`std::string`): Output file name. The binary
  file is written as
  `<memoryname>.0x<start_addr>-0x<end_addr>.<outfile>`.
- `MemoryDumper_trigger` (`bool`): Writing to this parameter
  triggers the dump.

The dumper must be bound to the main system router. It
discovers all memories in the system, finds their addresses,
and requests data via its initiator port. A target port must
also be bound; accessing its bound address triggers the dump.

## Router

The router is a simple address-decoding device. Initiators
and targets bind directly to the router's `target_socket` and
`initiator_socket` (both are multi-sockets).

### Address Discovery

The router uses CCI parameters to discover target device
addresses:

- `<target_name>.<socket_name>.address`
- `<target_name>.<socket_name>.size`
- `<target_name>.<socket_name>.relative_addresses`
- `<target_name>.<socket_name>.priority`

The `relative_addresses` flag is a boolean -- targets that
opt in receive addresses relative to their base address. The
default is relative addressing.

### Routing Order

Targets are sorted by `priority` (lower values match first).
Targets with equal priority maintain their bind order
(`std::stable_sort`). Overlapping addresses are allowed --
the first match is used. The default priority is 0.

### Convenience Functions

- `add_target(socket, base_address, size)` -- Sets
  appropriate CCI parameters (if not already set), sets
  `relative_addresses` to `true`, and handles socket binding.
- `add_initiator(socket)` -- Connects an initiator to the
  router and handles binding.

### PathIDExtension

The router adds a `gs::PathIDExtension` to transactions.
This extension holds a `std::vector` of port indices
(collectively a unique ID). The ID is composed by all routers
on the path that support this extension and can be used to
identify the originating initiator. The ID extension is held
in a thread-safe pool.

## Testing

Tests for each component are located under
`tests/base-components/`. After building, run:

```bash
# Run all tests
ctest --test-dir build -R base-components
```
