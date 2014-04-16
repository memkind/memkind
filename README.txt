Christopher Cantalupo <christopher.m.cantalupo@intel.com>
2014 April, 1

The numakind library extends libnuma with the ability to categorize
groups of numa nodes into different "kinds" of memory. It provides a
low level interface for generating inputs to mbind() and mmap(), and a
high level interface for heap management.  The heap management is
implemented with an extension to the jemalloc library which dedicates
"arenas" to each CPU node and kind of memory.  To use numakind,
jemalloc must be compiled with the --enable-numakind option.


Vishwanth Venkatesan <vishwanath.venkatesan@intel.com>
2014 April, 9

To generate documentation (HTML, pdf, and man):

user_prompt> make doc

Requires Doxygen:

user_prompt> sudo  yum install doxygen texlive-latex

