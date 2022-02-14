
[//]: # (SECTION 0)
## The GreenSocs SystemC simple components library.

This includes simple models such as routers, memories and exclusive monitor. The components are "Loosely timed" only. They support DMI where appropriate, and make use of CCI for configuration.

It also has several unit tests for memory, router and exclusive monitor.

[//]: # (SECTION 10)
## Information about building and using the base-components library
The base-components library depends on the libraries : Libgsutls, SystemC, RapidJSON, SystemCCI, Lua and GoogleTest.

[//]: # (SECTION 50)
## The GreenSocs component library memory
The memory component allows you to add memory when creating an object of type `Memory("name",size)`.

The memory component consists of a simple target socket :`tlm_utils::simple_target_socket<Memory> socket`

## The GreenSocs component library fallback memory
The fallback memory component allows you to add memory when creating an object of type `FallbackMemory("name",size)`.
The memory component consists of a simple target socket :`tlm_utils::simple_target_socket<Memory> socket`
The fallback memory allows loading from a CSV file, and will print a warning for each and every access. It will also refuse all DMI requests.

## The GreenSocs component library router
The  router is a simple device, the expectation is that initiators and targets are directly bound to the router's `target_socket` and `initiator_socket` directly (both are multi-sockets).
The router will use CCI param's to discover the addresses and size of target devices as follows:
 * `<target_name>.<socket_name>.address`
 * `<target_name>.<socket_name>.size`
 * `<target_name>.<socket_name>.mask_address`

A default tlm2 "simple target socket" will have the name `simple_target_socket_0` by default (this can be changed in the target).
The mask address is a boolean - targets which opt to have the router mask their address will receive addresses based from "address". Otherwise they will receive full addresses.

The router also offers `add_target(socket, base_address, size)` as a convenience, this will set appropriate param's (if they are not already set), and will set `mask_address` to be `true`.

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
