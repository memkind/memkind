# **README**

This is utils/memkind_malloc_wrapper/README.

libmemkind_malloc_wrapper.so is an interposer library for redirecting heap allocations to memkind.

By default MEMKIND_DEFAULT will be used.

LD_PRELOAD=libmemkind_malloc_wrapper.so app

# Environment variables

* **PMEM_KIND_PATH** - PMEM mount device path on host environment (If defined PMEM kind will be created and used)

* **MEMKIND_DAX_KMEM_ON** -  Setting MEMKIND_DAX_KMEM_ON to 1 results with using MEMKIND_DAX_KMEM instead of MEMKIND_DEFAULT