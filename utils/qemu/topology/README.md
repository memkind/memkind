# README

This is utils/qemu/topology/README.

XML files in this directory simulates different memory architectures.

- knm_all2all.xml - Knights Mill, cluster mode: All2All - memory: DRAM+MCDRAM
- knm_snc2.xml - Knights Mill, cluster mode: SNC-2 - memory: DRAM+MCDRAM
- knm_snc4.xml - Knights Mill, cluster mode: SNC-4 - memory: DRAM+MCDRAM
- clx_2_var1.xml - Cascade Lake, 2 sockets - memory DRAM+PMEM - PMEM on 1 socket
- clx_2_var2.xml - Cascade Lake, 2 sockets - memory DRAM+PMEM - PMEM on 2 socket
- clx_2_var3.xml - Cascade Lake, 2 sockets - memory DRAM+PMEM - PMEM on both sockets
- clx_4_var1.xml - Cascade Lake, 4 sockets - memory DRAM+PMEM - PMEM on 4 sockets
- clx_4_var2.xml - Cascade Lake, 4 sockets - memory DRAM+PMEM - PMEM on socket 2
- clx_4_var3.xml - Cascade Lake, 4 sockets - memory DRAM+PMEM - PMEM on sockets 1, 2 and 4
- clx_4_var4.xml - Cascade Lake, 4 sockets - memory DRAM+PMEM - PMEM on sockets 1 and 2 PMEMs on socket 4
