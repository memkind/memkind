#
#  Copyright (C) 2014, 2015 Intel Corporation.
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are met:
#  1. Redistributions of source code must retain the above copyright notice(s),
#     this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright notice(s),
#     this list of conditions and the following disclaimer in the documentation
#     and/or other materials provided with the distribution.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER(S) ``AS IS'' AND ANY EXPRESS
#  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
#  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO
#  EVENT SHALL THE COPYRIGHT HOLDER(S) BE LIABLE FOR ANY DIRECT, INDIRECT,
#  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
#  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
#  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
#  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
#  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

# targets for building rpm
version ?= 0.0.0
release ?= 1
arch = $(shell uname -p)

rpm: memkind-$(version).tar.gz
	rpmbuild $(rpmbuild_flags) $^ -ta

memkind-$(version).spec:
	@echo "$$memkind_spec" > $@
	cat ChangeLog >> $@

.PHONY: rpm

define memkind_spec
%if %{defined suse_version}
Name: libmemkind0
%else
Name: memkind
%endif
Summary: User Extensible Heap Manager
Version: $(version)
Release: $(release)
License: BSD-2-Clause
Group: System Environment/Libraries
URL: http://memkind.github.io/memkind
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
BuildRequires: automake libtool
%if %{defined suse_version}
BuildRequires: libnuma-devel
%else
BuildRequires: numactl-devel
%endif

Prefix: %{_prefix}
Prefix: %{_unitdir}
$(opt_obsolete)
$(opt_provides)

%define namespace memkind

%if %{defined suse_version}
%define docdir %{_defaultdocdir}/%{namespace}
%else
%define docdir %{_defaultdocdir}/%{namespace}-%{version}
%endif

%define statedir %{_localstatedir}/run/%{namespace}


# x86_64 is the only arch memkind will build due to its
# current dependency on SSE4.2 CRC32 instruction which
# is used to compute thread local storage arena mappings
# with polynomial accumulations via GCC's intrinsic _mm_crc32_u64
# For further info check:
# - /lib/gcc/<target>/<version>/include/smmintrin.h
# - https://gcc.gnu.org/bugzilla/show_bug.cgi?id=36095
# - http://en.wikipedia.org/wiki/SSE4
ExclusiveArch: x86_64

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
Requires: %{namespace} = %{version}-%{release}

%description devel
Install header files and development aids to link memkind library into
applications. The memkind library is an user extensible heap manager built on
top of jemalloc which enables control of memory characteristics and heap
partitioning on different kinds of memory. This software is being made
available for early evaluation. The memkind library should be considered beta:
bugs may exist. Feedback on design or implementation is greatly appreciated.

%prep
%setup -q -a 0 -n %{namespace}-%{version}

%build
# It is required that we configure and build the jemalloc subdirectory
# before we configure and start building the top level memkind directory.
# To ensure the memkind build step is able to discover the output
# of the jemalloc build we must create an 'obj' directory, and build
# from within that directory.
cd %{_builddir}/%{buildsubdir}/jemalloc/
echo %{version} > %{_builddir}/%{buildsubdir}/jemalloc/VERSION
test -f configure || %{__autoconf}
mkdir %{_builddir}/%{buildsubdir}/jemalloc/obj
ln -s %{_builddir}/%{buildsubdir}/jemalloc/configure %{_builddir}/%{buildsubdir}/jemalloc/obj/
cd %{_builddir}/%{buildsubdir}/jemalloc/obj
%configure --enable-autogen --with-jemalloc-prefix=jemk_ --enable-memkind \
           --enable-safe --enable-cc-silence --prefix=%{_prefix} \
           --includedir=%{_includedir} --libdir=%{_libdir} --bindir=%{_bindir} \
           --docdir=%{_docdir}/%{namespace} --mandir=%{_mandir} \
           CFLAGS="-std=gnu99"
%{__make} %{?_smp_mflags}

# Build memkind lib and tools
cd %{_builddir}/%{buildsubdir}
echo %{version} > %{_builddir}/%{buildsubdir}/VERSION
test -f configure || ./autogen.sh
%configure --enable-tls --prefix=%{_prefix} --libdir=%{_libdir} \
           --includedir=%{_includedir} --sbindir=%{_sbindir} \
           --mandir=%{_mandir} --docdir=%{_docdir}/%{namespace} \
           CFLAGS="-std=gnu99"
%{__make} %{?_smp_mflags}

%install
cd %{_builddir}/%{buildsubdir}
%{__make} DESTDIR=%{buildroot} install
%{__install} -d %{buildroot}/%{_unitdir}
%{__install} %{namespace}.service %{buildroot}/%{_unitdir}/%{namespace}.service
%if 0%{?suse_version} <= 1310
%{__install} -d %{buildroot}/%{statedir}
touch %{buildroot}/%{statedir}/node-bandwidth
%endif
rm -f %{buildroot}/%{_libdir}/lib%{namespace}.{l,}a
rm -f %{buildroot}/%{_libdir}/lib{numakind,autohbw}.*

%pre
%{service_add_pre %{namespace}.service}

%post
/sbin/ldconfig
%{service_add_post %{namespace}.service}

%preun
%{service_del_preun %{namespace}.service}

%postun
%{service_del_postun %{namespace}.service}
/sbin/ldconfig

%files
%defattr(-,root,root,-)
%license %{_docdir}/%{namespace}/COPYING
%doc %{_docdir}/%{namespace}/README
%doc %{_docdir}/%{namespace}/VERSION
%dir %{_docdir}/%{namespace}
%dir %{_unitdir}
%{_libdir}/lib%{namespace}.so.*
%{_bindir}/%{namespace}-hbw-nodes
%{_sbindir}/%{namespace}-pmtt
%{_unitdir}/%{namespace}.service
%if 0%{?suse_version} <= 1310
%ghost %dir %{statedir}
%ghost %{statedir}/node-bandwidth
%endif
%files devel
%defattr(-,root,root,-)
%{_includedir}/%{namespace}*.h
%{_includedir}/hbwmalloc.h
%{_libdir}/lib%{namespace}.so
%{_mandir}/man3/hbwmalloc.3.*
%{_mandir}/man3/%{namespace}*.3.*

%changelog
endef

export memkind_spec
