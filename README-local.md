

[//]: # (SECTION 0)

## LIBQEMU-CXX 

Libqemu-cxx encapsulates QEMU as a C++ object, such that it can be instanced (for instance) within a SystemC simulation framework.

[//]: # (SECTION 10)
## Information about building and using the libqemu-cxx library

The libgsutils library does not depend on any library.

[//]: # (SECTION 100)

The QEMU Library is dlopen'ed. In order to ensure that each instance is self contained, on Linux, a deep copy of the library is performed for every subsequent instance of the same library after the first. The copy is created in /tmp/qbox_lib.XXXXXX. The file is deleted once loaded. The result of this is that symbols from that library will not be accessible during debug. 

If it proves necessary to debug the temporary libraries, then recompile with the flag DEBUG_TMP_LIBRARIES defined. A warning will be issued on stdio identifying the temporary library which should be deleted once used.
