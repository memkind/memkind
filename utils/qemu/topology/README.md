# README

This is utils/qemu/topology/README.

XML files in this directory simulates different memory architectures.

- knm_all2all.xml - Knights Mill, cluster mode: All2All - memory: DRAM+MCDRAM
- knm_snc2.xml - Knights Mill, cluster mode: SNC-2 - memory: DRAM+MCDRAM
- knm_snc4.xml - Knights Mill, cluster mode: SNC-4 - memory: DRAM+MCDRAM
- clx_2_var1.xml - Cascade Lake, 2 sockets - memory DRAM+PMEM - PMEM on socket 0
- clx_2_var1_hmat.xml - Cascade Lake, 2 sockets - memory DRAM+PMEM - PMEM on socket 0 (HMAT support)
- clx_2_var2.xml - Cascade Lake, 2 sockets - memory DRAM+PMEM - PMEM on socket 1
- clx_2_var2_hmat.xml - Cascade Lake, 2 sockets - memory DRAM+PMEM - PMEM on socket 1 (HMAT support)
- clx_2_var2_emul_hbw.xml - emulated Cascade Lake, 2 sockets - memory DRAM - HBM on socket 1 (HMAT support)
- clx_2_var3.xml - Cascade Lake, 2 sockets - memory DRAM+PMEM - PMEM on both sockets
- clx_2_var3_hmat.xml - Cascade Lake, 2 sockets - memory DRAM+PMEM - PMEM on both sockets (HMAT support)
- clx_2_var3_emul_hbw.xml - emulated Cascade Lake, 2 sockets - memory DRAM - HBM on both sockets (HMAT support)
- clx_4_var1.xml - Cascade Lake, 4 sockets - memory DRAM+PMEM - PMEM on all 4 sockets
- clx_4_var1_hmat.xml - Cascade Lake, 2 sockets - memory DRAM+PMEM - PMEM on all 4 sockets (HMAT support)
- clx_4_var1_emul_hbw.xml - emulated Cascade Lake, 2 sockets - memory DRAM - HBM on all 4 sockets (HMAT support)
- clx_4_var2.xml - Cascade Lake, 4 sockets - memory DRAM+PMEM - PMEM on socket 1
- clx_4_var2_hmat.xml - Cascade Lake, 2 sockets - memory DRAM+PMEM - PMEM on socket 1 (HMAT support)
- clx_4_var2_emul_hbw.xml - emulated Cascade Lake, 2 sockets - memory DRAM - HBM on socket 1 (HMAT support)
- clx_4_var3.xml - Cascade Lake, 4 sockets - memory DRAM+PMEM - PMEM on sockets 0, 1 and 3
- clx_4_var3_hmat.xml - Cascade Lake, 2 sockets - memory DRAM+PMEM - PMEM on socket 0, 1 and 3 (HMAT support)
- clx_4_var3_emul_hbw.xml - emulated Cascade Lake, 2 sockets - memory DRAM - HBM on socket 0, 1 and 3 (HMAT support)
- clx_4_var4.xml - Cascade Lake, 4 sockets - memory DRAM+PMEM - PMEM on sockets 0 and 3
- clx_4_var4_hmat.xml - Cascade Lake, 2 sockets - memory DRAM+PMEM - PMEM on socket 0 and 3 (HMAT support)
- clx_4_var4_emul_hbw.xml - emulated Cascade Lake, 2 sockets - memory DRAM - HBM on socket 0 and 3 (HMAT support)
