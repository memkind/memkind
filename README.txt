Christopher Cantalupo <christopher.m.cantalupo@intel.com>
2014 March 12

This directory includes a re-factorization of the modifications made
for MCDRAM.  It was noted that no modifications have been made for the
case where previously freed mmap'ed regions are "recycled", only that
mbind() is called after mmap().  In the case where a new region is
returned by an allocation, the pages will not be populated, so mbind()
can be called on the outputs of jemalloc, rather than calling mbind()
within the jemalloc code.  This is because in the case where the pages
are populated then mbind() does not do anything:

       By default, mbind() only has an effect for new allocations; if
       the pages inside the range have been already touched before
       setting the policy, then the policy has no effect.  This
       default behavior may be overridden by the MPOL_MF_MOVE and
       MPOL_MF_MOVE_ALL flags described below.

The default behavior is to use a hash of the thread ID to choose the
arena where the memory will be allocated.  The table that this is
hashed into is length number of cpu's times four by default.  This
does not guarantee that collisions will not occur, but they will be
rare.  If a collision does occur between two threads that are not
located on the same CPU node, then the pages may be mixed between NUMA
nodes for those threads.  We need to use the experimental interface
and our own hash based on CPU node and kind to avoid this.

It seems that there has been an attempt to create a NUMA aware heap
manager by Patryk Kaminski at AMD.  You can find his white paper about
the implementation here:

http://developer.amd.com/wordpress/media/2012/10/NUMA_aware_heap_memory_manager_article_final.pdf

however I was not able to find the source code.  It can be seen in the
whitepaper that all attempts to implement this NUMA aware heap
management had issues that were not resolved.

