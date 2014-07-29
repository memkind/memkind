Christopher Cantalupo <christopher.m.cantalupo@intel.com>
2014 July 29


The example directory contains some example codes that use the
numakind interface and a Makefile to build them.  The simplest is the
hello_example.c which is a hello world variant.  The filter_example.c
shows how you would use high bandwidth memory to store a reduction of
a larger data set stored in DDR.  The stream_example.c is a variant of
the McCalpin stream benchmark that parses a command line option that
specifies which numakind to use.  Pass --help to stream_nk for the
available options.
