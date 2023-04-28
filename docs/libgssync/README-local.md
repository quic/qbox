

[//]: # (SECTION 0)
## LIBGSSYNC

The GreenSocs Synchronization library provides a number of different policies for synchronizing between an external simulator (typically QEMU) and SystemC.

These are based on a proposed standard means to handle the SystemC simulator. This library provides a backwards compatibility layer, but the patched version of SystemC will perform better.

[//]: # (SECTION 10)
## Information about building and using the libgssync library
The libgssync library depends on the libraries : base-components, libgsutils, SystemC, RapidJSON, SystemCCI, Lua and GoogleTest.

[//]: # (SECTION 50)
## Functionality of the synchronization library
In addition the library contains utilities such as an thread safe event (async_event) and a real time speed limited for SystemC.

### Suspend/Unsuspend interface

This patch adds four new basic functions to SystemC:

```
void sc_suspend_all(sc_simcontext* csc= sc_get_curr_simcontext())
void sc_unsuspend_all(sc_simcontext* csc= sc_get_curr_simcontext())
void sc_unsuspendable()
void sc_suspendable()
```

**suspend_all/unsuspend_all :**
This pair of functions requests the kernel to ‘atomically suspend’ all processes (using the same semantics as the thread suspend() call). This is atomic in that the kernel will only suspend all the processes together, such that they can be suspended and unsuspended without any side effects. Calling suspend_all(), and subsiquently calling unsuspend_all() will have no effect on the suspended status of an individual process. 
A process may call suspend_all() followed by unsuspend_all, the calls should be ‘paired’, (multiple calls to either suspend_all() or unsuspend_all() will be ignored).
Outside of the context of a process, it is the programmers responsibility to ensure that the calls are paired.
As a consequence, multiple calls to suspend_all() may be made (within separate process, or from within sc_main). So long as there have been more calls to suspend_all() than to unsuspend_all(), the kernel will suspend all processes.

_[note, this patch set does not add convenience functions, including those to find out if suspension has happened, these are expected to be layered ontop]_

**unsusbendable()/suspendable():**
This pair of functions provides an ‘opt-out’ for specific process to the suspend_all(). The consequence is that if there is a process that has opted out, the kernel will not be able to suspend_all (as it would no longer be atomic). 
These functions can only be called from within a process.
A process should only call suspendable/unsuspendable in pairs (multiple calls to either will be ignored).
_Note that the default is that a process is marked as suspendable._


**Use cases:**
_1 : Save and Restore_
For Save and Restore, the expectation is that when a save is requested, ‘suspend_all’ will be called. If there are models that are in an unsuspendable state, the entire simulation will be allowed to continue until such a time that there are no unsuspendable processes.

_2 : External sync_
When an external model injects events into a SystemC model (for instance, using an ‘async_request_update()’), time can drift between the two simulators. In order to maintain time, SystemC can be prevented from advancing by calling suspend_all(). If there are process in an unsuspendable state (for instance, processing on behalf of the external model), then the simulation will be allowed to continue. 
NOTE, an event injected into the kernel by an async_request_update will cause the kernel to execute the associated update() function (leaving the suspended state). The update function should arrange to mark any processes that it requires as unsuspendable before the end of the current delta cycle, to ensure that they are scheduled.

## List of options of sync policy parameter

The libgssync library allows you to set several values to the `p_sync_policy` parameter :
- `tlm2`
- `multithread`
- `multithread-quantum`
- `multithread-rolling`
- `multithread-adaptive`
- `multithread-unconstrained`
- `multithread-freerunning`

By default the parameter is set to `multithread-quantum`.

[//]: # (SECTION 100)
## The GreenSocs Synchronization Tests

It is possible to test the correct functioning of the different components with the tests that are proposed.

Once you have compiled your library, you will have a integration_tests folder in your construction directory.

In this test folder you will find several executable which each correspond to a test.
You can run the executable with :
```bash
./build/tests/integration_tests/<name_of_component>_test
```
