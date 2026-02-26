# nor_flash_backend

Native SystemC/TLM NOR flash backend for the QSPI controller.

## Architecture

```
┌─────────────────────┐        biflow_socket        ┌──────────────────────┐
│     QSPI module     │ ◄──────────────────────────► │   nor_flash_backend  │
│   (quic::qspi)      │                              │                      │
│                     │  last_write_fragment_signal  │  m_binary_content[]  │
│                     │ ─────────────────────────── ►│  selected profile    │
└─────────────────────┘                              │  (SFDP, ID, opcodes) │
                                                     └──────────────────────┘
```

The QSPI sends commands to the backend via `b_transport()`. The backend
processes them and enqueues response bytes back via `biflow_socket.enqueue()`.

The `last_write_fragment_signal` is driven by the QSPI to indicate whether
the current DMA write fragment is the last one in the chain. The backend uses
this to clear WEL (Write Enable Latch) only after the final fragment, matching
real NOR flash behaviour for multi-fragment DMA writes.

## Command set

| Opcode | Command | Behaviour |
|--------|---------|-----------|
| `0x03` | READ | Read from `m_binary_content` |
| `0x13` | READ4 | Read (4-byte address mode) |
| `0x0B/0x0C` | FAST_READ/FAST_READ4 | Fast read |
| `0x3B/0x3C` | DOUT_FAST_READ/DOUT_FAST_READ4 | Dual-output fast read |
| `0x6B/0x6C` | QOUT_FAST_READ/QOUT_FAST_READ4 | Quad-output fast read |
| `0xBB/0xBC` | DIO_FAST_READ/DIO_FAST_READ4 | Dual-I/O fast read |
| `0xEB/0xEC` | QIO_FAST_READ/QIO_FAST_READ4 | Quad-I/O fast read |
| `0xEC` | QIOR4 | Quad I/O read |
| `0x12` | PP4 | Page program (AND write into `m_binary_content`) |
| `0x34` | PP4_QUAD | Quad page program |
| `0x21` | ERASE_4K | Erase 4 KB sector (fill with `0xFF`) |
| `0x20` | ERASE_4K (3B) | Erase 4 KB sector |
| `0x52` | ERASE_32K | Erase 32 KB sector |
| `0xD8` | ERASE_SECTOR | Erase 64 KB sector |
| `0xDC` | ERASE4_SECTOR | Erase 64 KB sector (4-byte address) |
| `0xC7/0x60` | BULK_ERASE | Erase entire flash |
| `0x06` | WRITE_ENABLE | Set WEL in status register |
| `0x04` | WRITE_DISABLE | Clear WEL |
| `0x05` | READ_STATUS | Return status register |
| `0x70` | READ_FLAG_STATUS | Return flag status register |
| `0x65` | READ_ENHANCED_VOL_CFG | Return enhanced volatile config |
| `0x85` | READ_VOLATILE_CFG | Return volatile config |
| `0x9F` | READ_ID | Return device ID bytes |
| `0x5A` | READ_SFDP | Return SFDP table bytes |
| `0x01` | WRITE_STATUS | Write status register |
| `0x3E` | WRITE_STATUS_2 | Write MT25 status register 2 |
| `0x61` | WRITE_ENHANCED_VOL_CFG | Write enhanced volatile config |
| `0x81` | WRITE_VOLATILE_CFG | Write volatile config |
| `0xB7` | ENTER_4B_ADDR | No-op (acknowledged) |
| `0xB9/0xAB` | ENTER_DPD/RELEASE_DEEP_POWER_DOWN | No-op (acknowledged) |
| `0x66/0x99` | RESET_ENABLE/RESET | No-op (acknowledged) |
| `0x50` | CLEAR_FLAG_STATUS | Reset flag status register to `FSR_READY` |

### Write semantics

NOR flash is write-once per bit: `PP4` performs `memory[addr] &= data` (bits
can only be cleared, not set). Erase restores bits to `0xFF`.

WEL is cleared automatically after each write or erase operation. For DMA
multi-fragment writes, WEL is only cleared after the **last** fragment
(signalled by `last_write_fragment_signal = true`).

## Lua configuration

Configured via `nor_flash_backend_func()` in `qup.lua`:

```lua
nor_flash_backend_func({
    binary_path = "/path/to/singleimage.bin",
    flash_type  = "qspi",                       -- "qspi" (MT25) or "ospi" (MT35XU02GCBA1G12-0AUT)
})
```

| Parameter | Default | Description |
|-----------|---------|-------------|
| `binary_path` | `""` | Path to the NOR flash binary image |
| `flash_type` | `"qspi"` | Built-in MT25 QSPI or MT35X 2Gb OSPI profile |

The binary image is loaded once at `end_of_elaboration()` and kept in a
`std::vector<uint8_t>`. Writes and erases modify this in-memory buffer; the
file on disk is not modified.

`0xE8` and `0xB6` are intentionally rejected: their MT25 mode semantics are
not modelled. Unknown opcodes are rejected rather than acknowledged as no-ops.
