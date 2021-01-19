# README

This is utils/qemu/topology/README.

XML files in this directory simulates different memory architectures.
Note that systems marked with (*) are "emulated" and not based on real products.

| XML file name           | System / Mode           | Memory        | HMAT support | Notes                      |
| -----------------------:|:-----------------------:|:-------------:|:------------:|:--------------------------:|
| knm_all2all.xml         | Knights Mill / All2All  | DRAM + MCDRAM |              |                            |
| knm_snc2.xml            | Knights Mill / SNC-2    | DRAM + MCDRAM |              |                            |
| knm_snc4.xml            | Knights Mill / SNC-4    | DRAM + MCDRAM |              |                            |
| clx_2_var1.xml          | Cascade Lake 2 sockets  | DRAM + PMEM   |              | PMEM on socket 0           |
| clx_2_var1_emul_hbw.xml | Cascade Lake 2 sockets* | DRAM + HBM    | X            | HBM on socket 0            |
| clx_2_var1_hmat.xml     | Cascade Lake 2 sockets  | DRAM + PMEM   | X            | PMEM on socket 0           |
| clx_2_var2.xml          | Cascade Lake 2 sockets  | DRAM + PMEM   |              | PMEM on socket 1           |
| clx_2_var2_emul_hbw.xml | Cascade Lake 2 sockets* | DRAM + HBM    | X            | HBM on socket 1            |
| clx_2_var2_hmat.xml     | Cascade Lake 2 sockets  | DRAM + PMEM   | X            | PMEM on socket 1           |
| clx_2_var3.xml          | Cascade Lake 2 sockets  | DRAM + PMEM   |              | PMEM on both sockets       |
| clx_2_var3_emul_hbw.xml | Cascade Lake 2 sockets* | DRAM + HBM    | X            | HBM on both sockets        |
| clx_2_var3_hmat.xml     | Cascade Lake 2 sockets  | DRAM + PMEM   | X            | PMEM on both sockets       |
| clx_4_var1.xml          | Cascade Lake 4 sockets  | DRAM + PMEM   |              | PMEM on all 4 sockets      |
| clx_4_var1_emul_hbw.xml | Cascade Lake 4 sockets* | DRAM + HBM    | X            | HBM on all 4 sockets       |
| clx_4_var1_hmat.xml     | Cascade Lake 4 sockets  | DRAM + PMEM   | X            | PMEM on all 4 sockets      |
| clx_4_var2.xml          | Cascade Lake 4 sockets  | DRAM + PMEM   |              | PMEM on socket 1           |
| clx_4_var2_emul_hbw.xml | Cascade Lake 4 sockets* | DRAM + HBM    | X            | HBM on socket 1            |
| clx_4_var2_hmat.xml     | Cascade Lake 4 sockets  | DRAM + PMEM   | X            | PMEM on socket 1           |
| clx_4_var3.xml          | Cascade Lake 4 sockets  | DRAM + PMEM   |              | PMEM on sockets 0, 1 and 3 |
| clx_4_var3_emul_hbw.xml | Cascade Lake 4 sockets* | DRAM + HBM    | X            | HBM on sockets 0, 1 and 3  |
| clx_4_var3_hmat.xml     | Cascade Lake 4 sockets  | DRAM + PMEM   | X            | PMEM on sockets 0, 1 and 3 |
| clx_4_var4.xml          | Cascade Lake 4 sockets  | DRAM + PMEM   |              | PMEM on sockets 0 and 3    |
| clx_4_var4_emul_hbw.xml | Cascade Lake 4 sockets* | DRAM + HBM    | X            | HBM on sockets 0 and 3     |
| clx_4_var4_hmat.xml     | Cascade Lake 4 sockets  | DRAM + PMEM   | X            | PMEM on sockets 0 and 3    |
