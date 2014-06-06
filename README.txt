Christopher Cantalupo <christopher.m.cantalupo@intel.com>
2014 June 6


The numakind library extends libnuma with the ability to categorize
groups of NUMA nodes into different "kinds" of memory. It provides a
low level interface for generating inputs to mbind() and mmap(), and a
high level interface for heap management.  The heap management is
implemented with an extension to the jemalloc library which dedicates
"arenas" to each CPU node and kind of memory.  To use numakind,
jemalloc must be compiled with the --enable-numakind option.

Requires kernel patch introduced in Linux 3.12 that impacts
functionality of the NUMA system calls.

To use the interfaces for obtaining 2MB pages please be sure to follow
the instructions here:
    https://www.kernel.org/doc/Documentation/vm/hugetlbpage.txt
and pay particular attention to the use of the proc files:
    /proc/sys/vm/nr_hugepages
    /proc/sys/vm/nr_overcommit_hugepages
for enabling the kernel's huge page pool.

All existing tests pass both within the simics simulation environment
of KNL and when run on Xeon.  When run on Xeon the NUMAKIND_HBW_NODES
environment variable is used in conjunction with "numactl --membind"
to force standard allocations to one NUMA node and high bandwidth
allocations through a different NUMA node.

This software is being made available for early evaluation for
customers under non-disclosure agreements with Intel.  The numakind
library should be considered pre-alpha: bugs may exist and the
interfaces may be subject to change prior to alpha release.  Feedback
on design or implementation is greatly appreciated.

The numakind interface detailed in the numakind.3 man page should be
considered experimental, and is not necessarily going to be exposed to
customers on alpha release. Any feedback about the advantages or
disadvantages of the numakind interface over the hbwmalloc interface
described in the hbwmalloc.3 man page would be very helpful.
