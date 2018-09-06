# Memkind examples

The example directory contains some example codes that use the memkind
interface.

## PMEM

The pmem_*.c demonstrates how to create and use a file-backed memkind.

### pmem_kinds.c

A simple example showing how to create and destroy pmem kind with defined or unlimited size.

### pmem_malloc.c

Memory allocation example with possibility to exceed pmem kind size.

### pmem_malloc_unlimited.c

A similar example to pmem_malloc but with the use of unlimited kind size.

### pmem_usable_size.c

An example showing the difference between the expected and the actual allocation size.

### pmem_alignment.c

This example shows how to use memkind alignment and how it affects allocations.

### pmem_multithreads.c

Simple example how to use multithreading with pmem kinds.

## Other memkind examples

The simplest example is the hello_example.c which is a hello world
variant.  The filter_example.c shows how you would use high bandwidth
memory to store a reduction of a larger data set stored in DDR. There is
also an example of how to create user defined kinds.  This example
creates kinds which isolate allocations to a single NUMA node each
backed by a single arena.

The memkind_allocated example is simple usage of memkind in C++11 which
shows how memkind can be used to allocate objects, and consists of two files:
memkind_allocated.hpp - which is definition of template class that should be
inherited and parametrized by derived type (curiously recurring template
pattern), to let deriving class allocate their objects using specified kind.
memkind_allocated_example.cpp - which is usage example of this approach.
Logic of memkind_allocated is based on overriding operator new() in template,
and allocating memory on kind specified in new() parameter, or by overridable
static method getClassKind(). This implementation also supports alignment
specifier's (alignas() - new feature in C++11).
The downside of this approach is that it will work properly only if
memkind_allocated template is inherited once in inheritance chain (which
probably makes that not very useful for most scenarios). Other thing is that it
overriding class new() operator which can cause various problems if used
unwisely.