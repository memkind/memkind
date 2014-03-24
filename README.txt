Christopher Cantalupo <christopher.m.cantalupo@intel.com>
2014 March 21

This directory includes a re-factorization of the modifications made
for MCDRAM.  It was noted that no modifications have been made for the
case where previously freed mmap'ed regions are "recycled", only that
mbind() is called after mmap().  

The default behavior of jemalloc is to use a hash of the thread ID to
choose the arena where the memory will be allocated.  The table that
this is hashed into is length number of cpu's times four by default.
This does not guarantee that collisions will not occur, but they will
be rare.  If a collision does occur between two threads that are not
located on the same CPU node, then the pages may be mixed between NUMA
nodes for those threads.  

We have used the experimental interface and our own hash based on CPU
node and kind to avoid this.

It seems that there has been an attempt to create a NUMA aware heap
manager by Patryk Kaminski at AMD.  You can find his white paper about
the implementation here:

http://developer.amd.com/wordpress/media/2012/10/NUMA_aware_heap_memory_manager_article_final.pdf

however I was not able to find the source code.  It can be seen in the
whitepaper that all attempts to implement this NUMA aware heap
management had issues that were not resolved.

