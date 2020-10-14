# **README**

This is utils/memkind_malloc_wrapper/README.

libmemkind_malloc_wrapper.so is an interposer library for redirecting heap allocations to memkind.
libmemkind_malloc_wrapper_dbg.so is a debug interposer library for redirecting heap allocations to memkind:
    * Provides additional information regarding initialization/deinitialization of interposer library
    * If memkind build provide decorator support (./configure --enable-decorators), prints debug information on every allocation call

The library is for benchmarking/testing purposes only - the fork is not supported.

By default, MEMKIND_DEFAULT will be used.

LD_PRELOAD=libmemkind_malloc_wrapper.so app

# Environment variables

* **PMEM_KIND_PATH** - PMEM mount device path on host environment (If defined, PMEM kind will be created and used)

* **MEMKIND_DAX_KMEM_ON** -  Setting MEMKIND_DAX_KMEM_ON to 1 results with using MEMKIND_DAX_KMEM instead of MEMKIND_DEFAULT
