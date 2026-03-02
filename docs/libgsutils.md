# libgsutils -- Utilities Library

The GreenSocs utilities library provides CCI configuration
helpers, simple logging, test functions, and basic TLM port
types.

## CCI Helpers

### Marking Values as Consumed

Use `cci_get` to both retrieve a CCI preset value and mark
the parameter as consumed:

```cpp
template <typename T>
T cci_get(cci::cci_broker_handle broker, std::string name)
```

A variant with a default value is also available:

```cpp
template <typename T>
T cci_get_d(cci::cci_broker_handle broker, std::string name, T default_val)
```

### ConfigurableBroker

The `gs::ConfigurableBroker` is the primary configuration
mechanism in QBox. See [Configuration](configuration.md) for
full documentation on instantiation modes and parameter
handling.

## Testing

Tests are located under `tests/libgsutils/`. Run:

```bash
ctest --test-dir build -R libgsutils
```
