# **README**

This is utils/docker/README.

Scripts in this directory let Jenkins CI run a Docker container with ubuntu-18.04 environment to build, test and optionally measure test coverage of any pull request to memkind project, inside it.

# Files
*'docker_run_build_and_test.sh'*  is used to build and run tests of memkind.

*'docker_run_coverage.sh'*  is used for uploading coverage report on [Codecov.io](Codecov.io)


# Building and running the container

One can use the script *docker_run.sh* just to build and run tests.
```
$ ./docker_run.sh $PR_number $pmem_mounted_dir $build_dir
```
For doing it manually, following commands to build and run container can be used:

```
$ docker build -t memkind_container -f Dockerfile.ubuntu-18.04 --build-arg http_proxy=$http_proxy \
--build-arg https_proxy=$https_proxy .

$ docker run --rm --privileged=true -e http_proxy=$http_proxy -e https_proxy=$https_proxy \
--shm-size 10G memkind_container ./docker_run_build_and_test.sh -p $pull_request_no -d $pmem_dir \
-b $build_dir
```

**NOTE:** For information on above parameters, see *docker_run_build_and_test.sh*
