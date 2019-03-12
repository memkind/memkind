# **README**

This is utils/docker/README.

Scripts in this directory let run a Docker container with Ubuntu 18.04 environment
to build, test and optionally measure test coverage of any pull request to memkind project, inside it.

# Files
*'set_host_configuration.sh'*  is used to set hugepages configuration on host machine.

*'docker_install_tbb.sh'*  is used to install Intel Threading Building Blocks library.

*'docker_run_build_and_test.sh'*  is used to build and run tests of memkind.

*'docker_run_coverage.sh'*  is used for uploading coverage report on [Codecov.io](Codecov.io)


# Building and running the container

One can use the script *run_local.sh* just to build and run tests.
```
$ ./run_local.sh [ -p $PR_NUMBER ] [ -c $CODECOV_TOKEN ]
```

**NOTE:** For information on above parameters, see *run_local.sh*
