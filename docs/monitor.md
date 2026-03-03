# QBox Monitor

The QBox monitor is a web-based monitoring interface that
exposes the state of a running simulation through a set of
REST endpoints and WebSocket connections. It uses the Crow
web framework internally to serve an HTML dashboard and to
stream data in real time.

## Features

- Query the current simulation time (`/sc_time`).
- Pause and resume the simulation (`/pause`, `/continue`).
- Browse the SystemC object hierarchy and inspect CCI
  parameters (`/object/`, `/object/<name>`).
- View quantum-keeper status for multi-threaded simulations
  (`/qk_status`).
- Read memory through the TLM debug transport interface
  (`/transport_dbg/<addr>/<name>`).
- Connect to any `biflow_socket` in the design via WebSocket
  (`/biflow/<name>`), enabling browser-based VNC or serial
  console sessions.

During elaboration the monitor automatically discovers every
`biflow_multibindable` socket in the design and makes it
available over WebSocket.

## CCI Parameters

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `server_port` | `uint32_t` | `18080` | HTTP port the monitor listens on |
| `html_doc_template_dir_path` | `string` | (executable-relative `static/`) | Directory containing HTML templates |
| `html_doc_name` | `string` | `"monitor.html"` | Name of the main HTML document |
| `use_html_presentation` | `bool` | `true` | Serve the HTML dashboard; when false, the root URL returns a plain-text API listing |

## Example Configuration

```lua
platform["monitor_0"] = {
    moduletype = "monitor",
    server_port = 18080,
    use_html_presentation = true,
    html_doc_template_dir_path = "/path/to/html/templates",
    html_doc_name = "monitor.html",
}
```

After the simulation starts, open `http://localhost:18080/` in
a browser to access the dashboard.
