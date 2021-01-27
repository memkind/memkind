# SPDX-License-Identifier: BSD-2-Clause
# Copyright (C) 2014 - 2021 Intel Corporation.

# targets for building rpm
version ?= 0.0.0
release ?= 1
arch = $(shell uname -p)
name ?= memkind

rpm: $(name)-$(version).tar.gz
	rpmbuild $(rpmbuild_flags) $^ -ta

memkind-$(version).spec:
	@echo "$$memkind_spec" > $@
	cat ChangeLog >> $@

.PHONY: rpm

define memkind_spec
Summary: User Extensible Heap Manager
Name: $(name)
Version: $(version)
Release: $(release)
License: BSD-2-Clause
Group: System Environment/Libraries
URL: http://memkind.github.io/memkind
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
BuildRequires: automake libtool gcc-c++ unzip
%if %{defined suse_version}
BuildRequires: libnuma-devel
%else
BuildRequires: numactl-devel
%endif

%define daxctl_min_version 66
%if %{defined suse_version}
BuildRequires: libdaxctl-devel >= %{daxctl_min_version}
%else
BuildRequires: daxctl-devel >= %{daxctl_min_version}
%endif

Prefix: %{_prefix}
Prefix: %{_unitdir}
%if %{undefined suse_version}
Obsoletes: memkind
Provides: memkind libmemkind0
%endif

%define namespace memkind

%if %{defined suse_version}
%define docdir %{_defaultdocdir}/%{namespace}
%else
%define docdir %{_defaultdocdir}/%{namespace}-%{version}
%endif

# Upstream testing of memkind is done exclusively on x86_64; other archs
# are unsupported but may work.
ExclusiveArch: x86_64 ppc64 ppc64le s390x aarch64

# default values if version is a tagged release on github
%{!?commit: %define commit %{version}}
%{!?buildsubdir: %define buildsubdir %{namespace}-%{commit}}
Source0: https://github.com/%{namespace}/%{namespace}/archive/v%{commit}/%{buildsubdir}.tar.gz

%description
The memkind library is an user extensible heap manager built on top of
jemalloc which enables control of memory characteristics and a
partitioning of the heap between kinds of memory. The kinds of memory
are defined by operating system memory policies that have been applied
to virtual address ranges. Memory characteristics supported by
memkind without user extension include control of NUMA and page size
features. The jemalloc non-standard interface has been extended to
enable specialized arenas to make requests for virtual memory from the
operating system through the memkind partition interface. Through the
other memkind interfaces the user can control and extend memory
partition features and allocate memory while selecting enabled
features. This software is being made available for early evaluation.
Feedback on design or implementation is greatly appreciated.

%package devel
Summary: Memkind User Extensible Heap Manager development lib and tools
Group: Development/Libraries
Requires: %{name} = %{version}-%{release}
%if %{undefined suse_version}
Obsoletes: memkind-devel
Provides: memkind-devel
%endif

%description devel
Install header files and development aids to link memkind library into
applications.

%package tests
Summary: Extension to libnuma for kinds of memory - validation
Group: Validation/Libraries
Requires: %{name} = %{version}-%{release}

%description tests
memkind functional tests

%prep
%setup -q -a 0 -n $(name)-%{version}

%build
# It is required that we configure and build the jemalloc subdirectory
# before we configure and start building the top level memkind directory.
# To ensure the memkind build step is able to discover the output
# of the jemalloc build we must create an 'obj' directory, and build
# from within that directory.

cd %{_builddir}/%{buildsubdir}
echo %{version} > %{_builddir}/%{buildsubdir}/VERSION
./build.sh --prefix=%{_prefix} --includedir=%{_includedir} --libdir=%{_libdir} \
           --bindir=%{_bindir} --docdir=%{_docdir}/%{namespace} --mandir=%{_mandir} --sbindir=%{_sbindir}

%install
cd %{_builddir}/%{buildsubdir}
%{__make} DESTDIR=%{buildroot} install
%{__install} -d %{buildroot}$(memkind_test_dir)
%{__install} -d %{buildroot}/%{_unitdir}
%{__install} -d %{buildroot}/$(memkind_test_dir)/python_framework
%{__install} test/.libs/* test/*.sh test/*.ts test/*.py %{buildroot}$(memkind_test_dir)
%{__install} test/python_framework/*.py %{buildroot}/$(memkind_test_dir)/python_framework
rm -f %{buildroot}$(memkind_test_dir)/libautohbw.*
rm -f %{buildroot}/%{_libdir}/lib%{namespace}.{l,}a
rm -f %{buildroot}/%{_libdir}/libautohbw.{l,}a

%pre

%post
/sbin/ldconfig

%preun

%postun
/sbin/ldconfig

%files
%defattr(-,root,root,-)
%license %{_docdir}/%{namespace}/COPYING
%doc %{_docdir}/%{namespace}/README
%doc %{_docdir}/%{namespace}/VERSION
%dir %{_docdir}/%{namespace}
%{_libdir}/lib%{namespace}.so.*
%{_libdir}/libautohbw.so.*
%{_bindir}/%{namespace}-auto-dax-kmem-nodes
%{_bindir}/%{namespace}-hbw-nodes

%files devel
%defattr(-,root,root,-)
%{_includedir}
%{_includedir}/hbwmalloc.h
%{_includedir}/hbw_allocator.h
%{_includedir}/memkind_allocator.h
%{_includedir}/pmem_allocator.h
%{_libdir}/lib%{namespace}.so
%{_libdir}/libautohbw.so
%{_libdir}/pkgconfig/memkind.pc
%{_includedir}/%{namespace}.h
%{_mandir}/man1/memkind-auto-dax-kmem-nodes.1.*
%{_mandir}/man1/memkind-hbw-nodes.1.*
%{_mandir}/man3/hbwmalloc.3.*
%{_mandir}/man3/hbwallocator.3.*
%{_mandir}/man3/pmemallocator.3.*
%{_mandir}/man3/%{namespace}*.3.*
%{_mandir}/man7/autohbw.7.*

%files tests
%defattr(-,root,root,-)
$(memkind_test_dir)/all_tests
$(memkind_test_dir)/background_threads_test
${memkind_test_dir}/environ_err_dax_kmem_malloc_positive_test
${memkind_test_dir}/environ_err_dax_kmem_malloc_test
$(memkind_test_dir)/environ_err_hbw_malloc_test
$(memkind_test_dir)/environ_max_bg_threads_test
${memkind_test_dir}/dax_kmem_test
$(memkind_test_dir)/decorator_test
$(memkind_test_dir)/locality_test
$(memkind_test_dir)/freeing_memory_segfault_test
$(memkind_test_dir)/gb_page_tests_bind_policy
$(memkind_test_dir)/filter_memkind
$(memkind_test_dir)/hello_hbw
$(memkind_test_dir)/hello_memkind
$(memkind_test_dir)/hello_memkind_debug
$(memkind_test_dir)/memkind_allocated
$(memkind_test_dir)/memkind_cpp_allocator
$(memkind_test_dir)/memkind_get_stat
$(memkind_test_dir)/memkind_stat_test
$(memkind_test_dir)/autohbw_candidates
${memkind_test_dir}/pmem_kinds
${memkind_test_dir}/pmem_malloc
${memkind_test_dir}/pmem_malloc_unlimited
${memkind_test_dir}/pmem_usable_size
${memkind_test_dir}/pmem_alignment
${memkind_test_dir}/pmem_and_dax_kmem_kind
${memkind_test_dir}/pmem_and_default_kind
${memkind_test_dir}/pmem_config
$(memkind_test_dir)/pmem_detect_kind
${memkind_test_dir}/pmem_multithreads
${memkind_test_dir}/pmem_multithreads_onekind
$(memkind_test_dir)/pmem_free_with_unknown_kind
${memkind_test_dir}/pmem_cpp_allocator
${memkind_test_dir}/pmem_test
${memkind_test_dir}/allocator_perf_tool_tests
${memkind_test_dir}/perf_tool
${memkind_test_dir}/autohbw_test_helper
${memkind_test_dir}/trace_mechanism_test_helper
$(memkind_test_dir)/memkind-afts.ts
$(memkind_test_dir)/memkind-afts-ext.ts
$(memkind_test_dir)/memkind-slts.ts
$(memkind_test_dir)/memkind-perf.ts
$(memkind_test_dir)/memkind-perf-ext.ts
$(memkind_test_dir)/memkind-pytests.ts
$(memkind_test_dir)/performance_test
$(memkind_test_dir)/test.sh
$(memkind_test_dir)/test_dax_kmem.sh
$(memkind_test_dir)/hbw_detection_test.py
$(memkind_test_dir)/dax_kmem_env_var_test.py
$(memkind_test_dir)/autohbw_test.py
$(memkind_test_dir)/trace_mechanism_test.py
$(memkind_test_dir)/python_framework
$(memkind_test_dir)/python_framework/cmd_helper.py
$(memkind_test_dir)/python_framework/huge_page_organizer.py
$(memkind_test_dir)/python_framework/__init__.py
$(memkind_test_dir)/draw_plots.py
$(memkind_test_dir)/run_alloc_benchmark.sh
$(memkind_test_dir)/alloc_benchmark_hbw
$(memkind_test_dir)/alloc_benchmark_glibc
$(memkind_test_dir)/alloc_benchmark_tbb
$(memkind_test_dir)/alloc_benchmark_pmem
$(memkind_test_dir)/fragmentation_benchmark_pmem
$(memkind_test_dir)/defrag_reallocate

%exclude $(memkind_test_dir)/*.pyo
%exclude $(memkind_test_dir)/*.pyc
%exclude $(memkind_test_dir)/python_framework/*.pyo
%exclude $(memkind_test_dir)/python_framework/*.pyc

%changelog
endef

export memkind_spec
