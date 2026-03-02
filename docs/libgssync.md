# libgssync -- Synchronization Library

The GreenSocs synchronization library provides policies for
synchronizing between an external simulator (typically QEMU)
and SystemC. It also includes utilities such as a thread-safe
event (`async_event`) and a real-time speed limiter for
SystemC.

These are based on a proposed standard for handling the SystemC
simulator. The library provides a backwards-compatibility
layer, but a patched version of SystemC will perform better.

## Suspend/Unsuspend Interface

Four functions extend the SystemC kernel:

```cpp
void sc_suspend_all()
void sc_unsuspend_all()
void sc_unsuspendable()
void sc_suspendable()
```

### suspend_all / unsuspend_all

This pair requests the kernel to atomically suspend all
processes (using the same semantics as the thread `suspend()`
call). The kernel will only suspend all processes together, so
they can be suspended and unsuspended without side effects.
Calling `suspend_all()` followed by `unsuspend_all()` has no
effect on an individual process's suspended status.

Calls should be paired. Multiple calls to `suspend_all()` from
separate processes or `sc_main` are allowed -- the kernel
suspends all processes as long as there have been more
`suspend_all()` calls than `unsuspend_all()` calls.

### unsuspendable / suspendable

These provide an opt-out for specific processes. If any process
has opted out, the kernel cannot `suspend_all` (since it would
no longer be atomic). These can only be called from within a
process, should be paired, and the default state is
suspendable.

### Use Cases

**Save and Restore:** When a save is requested, `suspend_all`
is called. If any models are in an unsuspendable state, the
simulation continues until all processes are suspendable.

**External Sync:** When an external model injects events into
SystemC (e.g., via `async_request_update()`), time can drift
between simulators. Calling `suspend_all()` prevents SystemC
from advancing. An event injected via `async_request_update`
causes the kernel to execute the associated `update()`
function. The update function should mark any required
processes as unsuspendable before the end of the current delta
cycle.

## Synchronization Policies

The `p_sync_policy` parameter accepts the following values:

| Policy | Description |
|--------|-------------|
| `tlm2` | Standard TLM2 mode. No multi-thread handling in the quantum keeper. Use with `COROUTINE` TCG mode only. |
| `multithread` | Basic multi-threaded quantum keeper. Keeps everything within approximately 2 quantums. |
| `multithread-quantum` | Closer to TLM behavior -- processes do not advance until all reach the quantum boundary. |
| `multithread-rolling` | Rolling synchronization. |
| `multithread-adaptive` | Adaptive synchronization. |
| `multithread-unconstrained` | Allows QEMU to run at its own pace. |
| `multithread-freerunning` | Free-running mode. |

The default is `multithread-quantum`.

See [libqbox](libqbox.md#tlm2-quantum-keeper-synchronization-mode)
for how sync policies interact with QEMU TCG threading modes.

## Testing

Tests are located under `tests/libgssync/`. Run them with:

```bash
ctest --test-dir build -R libgssync
```
