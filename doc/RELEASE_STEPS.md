# Memkind release steps

This document contains all the steps required to make a new release of memkind.

## Release candidate

It's a good idea to create a Release Candidate before an actual release.
To see an example how this should be done, look at one of the previous release candidates:
[1.14.0-rc1 release commit](https://github.com/memkind/memkind/commit/6dee284d1b380adcca98b4d82a8afa93e938c513).

## Prepare a release

- add info with changes in the [Changelog](../ChangeLog)
- add `VERSION` file with memkind version to-be-released
- update `MEMKIND_VERSION_*` defines in [src/memkind.c](../src/memkind.c#L4-L6)
- commit these changes and add annotated tag for this commit
- push the commit (along with the tag) to the upstream

To see an example how this change should be done, look at one of the previous releases:
[1.14.0 release commit](https://github.com/memkind/memkind/commit/d1eb0a391653dab6939497cf589bfc83e56e15b6).

## Make a GitHub release

It has to be based on a newly created tag.
See [GitHub's guide on creating releases](https://docs.github.com/en/github/administering-a-repository/releasing-projects-on-github/managing-releases-in-a-repository).

## Clean up after release

Remove VERSION file after making a release.
To see an example how this change should be done, look at
[this commit](https://github.com/memkind/memkind/commit/5fc2555105a600da34ad7fc28b03f993439371ce).

## When release is public

1. Update the web page, if needed: [pmem.io/memkind](https://pmem.io/memkind).
The web page code for manpages can be found on [pmem.github.io repository](https://github.com/pmem/pmem.github.io/tree/main/content/memkind).

2. For major/minor releases add a new release note, like
[in this commit](https://github.com/pmem/pmem.github.io/commit/c6658b6cc2b2d2306ee1b6decf5bd762d734df03).

3. Inform about the release
Use the google group to inform pmem community about the release, similarly to
[this post](https://groups.google.com/g/pmem/c/vE8iWGRVG4U/m/6qs7Z5x_BAAJi)

4. Request an update of the memkind package in Linux distros.
For RedHat request, report a "bug" - similarly to [this bug report](https://bugzilla.redhat.com/show_bug.cgi?id=2035221).
This will update packages both for Fedora and RedHat distros.
