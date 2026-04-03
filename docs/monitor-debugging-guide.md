# QBox Monitor — CLI Debugging Guide

The QBox monitor exposes REST and WebSocket endpoints that can be used
from the command line for live debugging of a running simulation. This
is particularly useful when diagnosing hangs, interrupt storms, or
unexpected CPU states without needing the QEMU monitor window.

## Prerequisites

- A running simulation with the monitor module enabled
- `curl` (REST endpoints)
- `websocat` (WebSocket/QMP commands) — install with `brew install websocat`

Throughout this guide the monitor port is assumed to be **18088**.
Adjust to match your `server_port` CCI parameter.

---

## 1. Quick Health Check

```bash
# Current simulation time (is it advancing?)
curl -s http://localhost:18088/sc_time
# {"sc_time_stamp":516.179}

# Check again after a couple of seconds
sleep 2 && curl -s http://localhost:18088/sc_time
```

If `sc_time_stamp` is not advancing, the simulation is stuck.

---

## 2. CPU State — Quantum-Keeper Status

```bash
# Which CPUs are running vs idle?
curl -s http://localhost:18088/qk_status | python3 -c "
import json, sys
for qk in json.load(sys.stdin):
    if 'cpu' in qk['name']:
        print(f\"{qk['name']:50s} {qk['state']:10s} local={qk['local_time']}\")
"
```

Example output:
```
platform.cpu_0.qk          RUNNING    local=1007631952 us
platform.cpu_1.qk          IDLE       local=1007631862 us
...
```

- **RUNNING** — CPU is actively executing instructions
- **IDLE** — CPU is in WFI/WFE (waiting for an interrupt or event)

If all CPUs are IDLE the simulation can still advance (virtual time
progresses), but the guest is not executing any code.

---

## 3. Inspecting SystemC Objects and CCI Parameters

```bash
# List all top-level objects
curl -s http://localhost:18088/object/ | python3 -m json.tool

# Inspect a specific module (e.g. the GIC)
curl -s http://localhost:18088/object/platform.gic_0 | python3 -m json.tool

# Inspect a CPU
curl -s http://localhost:18088/object/platform.cpu_0 | python3 -m json.tool
```

The response includes `child_objects`, `cci_params`, and `debug_ports`.

---

## 4. Reading Memory via Debug Transport

```bash
# Read 4 bytes at address <decimal_addr> via <socket_name>
curl -s http://localhost:18088/transport_dbg/<addr>/<socket>
```

The address is in **decimal** and the socket is the full SystemC
hierarchical name of a TLM target socket. The read goes through
TLM `transport_dbg`, which is non-intrusive (does not advance
simulation time).

### Example — Reading through the platform router

```bash
# Read 4 bytes at physical address 0x17A00000 (GIC distributor)
# 0x17A00000 = 398458880 decimal
curl -s http://localhost:18088/transport_dbg/398458880/platform.router.target_socket
```

### Limitation — GIC debug transport

The QEMU-backed GIC does not properly forward the address offset in
debug transport reads. All reads return the value at offset 0
(GICD_CTLR). Use QMP/HMP commands instead for GIC register reads
(see Section 6).

---

## 5. Listing Biflow Sockets

```bash
curl -s http://localhost:18088/biflows
# {"biflows":["platform.backend_router_17.socket",
#             "platform.qmp.qmp_socket.qmp_socket_router",
#             "platform.qmp_json.qmp_socket.qmp_socket_router"]}
```

The `qmp_json` biflow is the QMP (QEMU Machine Protocol) interface.

---

## 6. Sending QEMU Monitor Commands via QMP

This is the most powerful feature for debugging. You can send any QEMU
human monitor command through the QMP WebSocket.

### Basic pattern

```bash
echo '{"execute": "human-monitor-command", "arguments": {"command-line": "CMD"}}' \
  | timeout 3 websocat -n1 \
    ws://localhost:18088/biflow/platform.qmp_json.qmp_socket.qmp_socket_router
```

> **Note:** The biflow replays a buffer of recent output to new
> connections. Parse responses carefully — you may see old data before
> your result.

### Get CPU register state

```bash
# Registers for the currently selected CPU (usually CPU 0)
echo '{"execute": "human-monitor-command", "arguments": {"command-line": "info registers"}}' \
  | timeout 3 websocat -n1 \
    ws://localhost:18088/biflow/platform.qmp_json.qmp_socket.qmp_socket_router \
  2>&1 | python3 -c "
import sys, re, json
raw = sys.stdin.read()
returns = re.findall(r'\"return\":\s*\"((?:[^\"\\\\\"]|\\\\.)*)\"', raw)
for r in returns:
    decoded = r.replace(r'\r\n', '\n')
    if 'PC=' in decoded:
        print(decoded)
"
```

### Switch CPU and read registers

```bash
(sleep 0.3
 echo '{"execute": "human-monitor-command", "arguments": {"command-line": "cpu 4"}}'
 sleep 0.3
 echo '{"execute": "human-monitor-command", "arguments": {"command-line": "info registers"}}'
 sleep 1
) | timeout 5 websocat \
    ws://localhost:18088/biflow/platform.qmp_json.qmp_socket.qmp_socket_router \
  2>&1 > /tmp/qmp_regs.txt
```

### Read physical memory

```bash
# Read 4 words at physical address 0x17A00000
echo '{"execute": "human-monitor-command", "arguments": {"command-line": "xp /4xw 0x17A00000"}}' \
  | timeout 3 websocat -n1 \
    ws://localhost:18088/biflow/platform.qmp_json.qmp_socket.qmp_socket_router
```

### Read virtual memory (requires a CPU context)

```bash
echo '{"execute": "human-monitor-command", "arguments": {"command-line": "x /4xw 0xffffff80600b8000"}}' \
  | timeout 3 websocat -n1 \
    ws://localhost:18088/biflow/platform.qmp_json.qmp_socket.qmp_socket_router
```

### Enable/disable QEMU trace logging

```bash
# Enable interrupt logging
echo '{"execute": "human-monitor-command", "arguments": {"command-line": "log int"}}' \
  | timeout 3 websocat -n1 \
    ws://localhost:18088/biflow/platform.qmp_json.qmp_socket.qmp_socket_router

# Disable logging
echo '{"execute": "human-monitor-command", "arguments": {"command-line": "log none"}}' \
  | timeout 3 websocat -n1 \
    ws://localhost:18088/biflow/platform.qmp_json.qmp_socket.qmp_socket_router
```

### List all CPUs

```bash
echo '{"execute": "human-monitor-command", "arguments": {"command-line": "info cpus"}}' \
  | timeout 3 websocat -n1 \
    ws://localhost:18088/biflow/platform.qmp_json.qmp_socket.qmp_socket_router
```

---

## 7. Useful One-Liners

### Is the system making progress?

```bash
for i in 1 2 3; do
  curl -s http://localhost:18088/sc_time
  echo
  sleep 2
done
```

### Sample CPU 0 PC repeatedly (detect loops)

```bash
for i in $(seq 1 5); do
  echo '{"execute": "human-monitor-command", "arguments": {"command-line": "info registers"}}' \
    | timeout 3 websocat -n1 \
      ws://localhost:18088/biflow/platform.qmp_json.qmp_socket.qmp_socket_router \
    2>&1 | grep -oE 'PC=[0-9a-f]+' | tail -1
  sleep 1
done
```

### Full GIC distributor dump via QMP

```bash
# Read GICD_ISPENDR registers to see pending SPIs
for i in $(seq 0 13); do
  off=$((0x200 + i * 4))
  hex=$(printf "0x17A00%03x" $off)  # adjust base address
  echo '{"execute":"human-monitor-command","arguments":{"command-line":"xp /1xw '$hex'"}}' \
    | timeout 3 websocat -n1 \
      ws://localhost:18088/biflow/platform.qmp_json.qmp_socket.qmp_socket_router \
    2>&1 | grep -oE '0x[0-9a-f]+'
done
```

---

## 8. Web Dashboard

Open `http://localhost:18088/` in a browser for:

- **Object hierarchy browser** — click through modules, view CCI parameters
- **Terminal emulator** — connect to serial biflows (xterm.js)
- **VNC client** — connect to QEMU display (noVNC)
- **QMP command interface** — send commands interactively

---

## 9. Warnings

- **Do not use `/pause`** — pausing the simulation via the REST
  endpoint can cause unexpected side effects in the SystemC scheduler
  and may not resume cleanly.
- **Biflow buffer replay** — WebSocket connections receive a replay of
  recent data (up to 1000 characters). Parse responses carefully to
  distinguish old data from fresh responses.
- **GIC debug transport** — reads through the GIC's TLM debug
  transport do not respect the address offset; all reads return the
  value at offset 0. Use QMP `xp` commands instead.
