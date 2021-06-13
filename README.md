

[//]: # (SECTION 0)
## LIBQEMU-CXX 

Libqemu-cxx encapsulates QEMU as a C++ object, such that it can be instanced (for instance) within a SystemC simulation framework.

[//]: # (SECTION 0 AUTOADDED)


[//]: # (SECTION 1)


[//]: # (SECTION 1 AUTOADDED)


# GreenSocs Build and make system

# How to build
> 
> This project may be built using cmake
> ```
> cmake -B BUILD;cd BUILD; make -j
> ```
> 
> cmake version 3.14 or newer is required. This can be downloaded and used as follows
> ```
> curl -L https://github.com/Kitware/CMake/releases/download/v3.20.0-rc4/cmake-3.20.0-rc4-linux-x86_64.tar.gz | tar -zxf -
> ./cmake-3.20.0-rc4-linux-x86_64/bin/cmake
> ```
> 
>

## details

This project uses CPM https://github.com/cpm-cmake/CPM.cmake in order to find, and/or download missing components. In order to find locally installed SystemC, you may use the standards SystemC environment variables: `SYSTEMC_HOME` and `CCI_HOME`.
CPM will use the standard CMAKE `find_package` mechanism to find installed packages https://cmake.org/cmake/help/latest/command/find_package.html
To specify a specific package location use `<package>_ROOT`
CPM will also search along the CMAKE_MODULE_PATH

Sometimes it is convenient to have your own sources used, in this case, use the `CPM_<package>_SOURCE_DIR`.
Hence you may wish to use your own copy of SystemC CCI 
```
cmake -B BUILD -DCPM_SystemCCCI_SOURCE=/path/to/your/cci/source`
```


NB, CMake holds a cache of compiled modules in ~/.cmake/ Sometimes this can confuse builds. If you seem to be picking up the wrong version of a module, then it may be in this cache. It is perfectly safe to delete it.

### Common CMake options
`CMAKE_INSTALL_PREFIX` : Install directory for the package and binaries.
`CMAKE_BUILD_TYPE`     : DEBUG or RELEASE


The library assumes the use of C++14, and is compatible with SystemC versions from SystemC 2.3.1a.


## More documentation

More documentation, including doxygen generated API documentation can be found in the `/docs` directory.



[//]: # (SECTION 100)

The QEMU Library is dlopen'ed. In order to ensure that each instance is self contained, on Linux, a deep copy of the library is performed for every subsequent instance of the same library after the first. The copy is created in /tmp/qbox_lib.XXXXXX. The file is deleted once loaded. The result of this is that symbols from that library will not be accessible during debug. 

If it proves necessary to debug the temporary libraries, then recompile with the flag DEBUG_TMP_LIBRARIES defined. A warning will be issued on stdio identifying the temporary library which should be deleted once used.

[//]: # (PROCESSED BY doc_merge.pl)
