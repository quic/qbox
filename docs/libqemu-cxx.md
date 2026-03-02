# libqemu-cxx -- QEMU C++ Wrapper

Libqemu-cxx encapsulates QEMU as a C++ object so that it
can be instantiated within a SystemC simulation framework.

## Library Loading

The QEMU library is loaded via `dlopen`. To ensure that each
instance is self-contained, on Linux a deep copy of the
library is performed for every subsequent instance of the
same library after the first. The copy is created as
`/tmp/qbox_lib.XXXXXX` and deleted once loaded.

As a consequence, symbols from that library are not
accessible during debug sessions.

## Debugging Temporary Libraries

If you need to debug the temporary libraries, recompile with
the `DEBUG_TMP_LIBRARIES` flag defined. A warning will be
printed to stdio identifying the temporary library path. The
temporary file will not be deleted in this mode, and should
be cleaned up manually after use.
