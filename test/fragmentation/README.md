# **README**

This is test/fragmentation/README.

The program logging measurement of allocations.

The program in pmem_fragmentation.c takes, as arguments:
* pmem_path - path where file-backed memory kind should be created
* pmem_size - file-backed memory kind size limit, given in megabytes
* pmem_memory_usage_policy - 0 - MEMKIND_MEM_USAGE_POLICY_DEFAULT, 1 - MEMKIND_MEM_USAGE_POLICY_CONSERVATIVE
* test_time, - time how long binary ,givne in seconds
* output_file - file where results are saved

Note:
* test_time is given in seconds

For example:

	./fragmentation_test /mnt/pmem 17 0 60 output.txt

This will create PMEM kind limited with 17 MB in /mnt/pmem/ with default memory usage policy
and save results in output.txt file after 60 seconds.
