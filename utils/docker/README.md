# **README**

This is utils/docker/README.

Scripts in this directory let run a Docker container with Ubuntu 20.04, CentOS 7 and Fedora 32 environment
to build, test and optionally measure test coverage of any pull request to memkind project, inside it.

# Environment variables

* **MEMKIND_HOST_WORKDIR** - Defines the absolute path to memkind project root directory on host environment.
    **This variable must be defined.**

* **CODECOV_TOKEN** - Codecov token for memkind repository to upload the coverage results.

* **TEST_SUITE_NAME** - Name of test suite (possible values are HBW/PMEM/DAX_KMEM).

* **PMEM_HOST_PATH** - PMEM mount device path on host environment (useful for testing PMEM configuration). Default value of **PMEM_HOST_PATH** is
*'/tmp/'*.

* **NDCTL_LIBRARY_VERSION** - ndctl library version.
For fully testing MEMKIND_DAX_KMEM, ndctl library version tag must be passed as parameter,
see https://github.com/pmem/ndctl/tags.

* **ENABLE_HWLOC** - install hwloc library.
For fully testing MEMKIND_HBW*, MEMKIND_HIGHEST_BANDWIDTH, MEMKIND_LOWEST_LATENCY, MEMKIND_*LOCAL hwloc library version must be installed.
Setting **ENABLE_HWLOC** to 1 runs the installation of hwloc library.

* **TBB_LIBRARY_VERSION** - Intel Threading Building Blocks library version.
For testing Threading Building Blocks, TBB library version tag must be passed as parameter,
see https://github.com/01org/tbb/tags.

* **HOG_MEMORY** - Controls behavior of memkind with regards to returning memory to underlying OS. Setting **HOG_MEMORY** to 1 causes
memkind to not release memory to OS in anticipation of memory reuse soon. For PMEM memory will be released only after calling memkind_destroy_kind()

* **QEMU_TEST** - Setting **QEMU_TEST** results in the execution of tests designated for the QEMU environment.

# Files
*'docker_run_build.sh'*  is used to build of memkind.

*'docker_run_coverage.sh'*  is used for uploading coverage report on [Codecov.io](Codecov.io)

*'docker_run_test.sh'*  is used to run tests of memkind.

*'docker_install_hwloc.sh'*  is used to install hwloc library.

*'docker_install_libvirt.sh'*  is used to install libvirt library.

*'docker_install_ndctl.sh'*  is used to install ndctl library.

*'docker_install_qemu.sh'*  is used to install QEMU library.

*'docker_install_tbb.sh'*  is used to install Intel Threading Building Blocks library.

*'set_host_configuration.sh'*  is used to set hugepages configuration on host machine.

# Building and running the container

One can use the script *run_local.sh* just to build and run tests, e.g.:

```
$ ./run_local.sh Dockerfile.ubuntu-20.04
```

**Note:**

Others class group must have write permission to MEMKIND_HOST_WORKDIR to use the run_local script.
