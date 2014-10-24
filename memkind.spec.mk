#
#  Copyright (C) 2014 Intel Corporation.
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
version = 0.0.0
release = 1
arch = $(shell uname -p)
topdir = $(HOME)/rpmbuild
rpm = $(topdir)/RPMS/$(arch)/memkind-$(version)-$(release)-$(arch).rpm
rpmbuild_flags = -E '%define _topdir $(topdir)'
ifeq ($(jemalloc_installed),true)
	rpmbuild_flags += -E '%define jemalloc_installed true'
endif

rpm: $(rpm)

$(rpm): memkind-$(version).tar.gz
	rpmbuild $(rpmbuild_flags) $^ -ta

memkind-$(version).spec:
	@echo "$$memkind_spec" > $@

.PHONY: rpm

define memkind_spec
Summary: Extention to libnuma for kinds of memory
Name: memkind
Version: $(version)
Release: $(release)
License: See COPYING
Group: System Environment/Libraries
Vendor: Intel Corporation
URL: http://github.com/memkind/memkind
Source0: memkind-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root
BuildRequires:  gcc-c++
%if %{defined suse_version}
BuildRequires: libnuma-devel
%else
BuildRequires: numactl-devel
%endif
%if ! %{defined jemalloc_installed}
BuildRequires: jemalloc-devel
%endif

%description
The memkind library extends libnuma with the ability to categorize
groups of NUMA nodes into different "kinds" of memory. It provides a
low level interface for generating inputs to mbind() and mmap(), and a
high level interface for heap management.  The heap management is
implemented with an extension to the jemalloc library which dedicates
"arenas" to each CPU and kind of memory.  Additionally the heap is
partitioned so that freed memory segments of different kinds are not
coalesced.  The memkind library enables page size selection at
allocationt time.  To use memkind, jemalloc must be compiled with the
--enable-memkind and --with-jemalloc-prefix=je_ options.

%prep

%setup

%package devel
Summary: Extention to libnuma for kinds of memory - development
Group: Development/Libraries

%description devel
The memkind library extends libnuma with the ability to categorize
groups of NUMA nodes into different "kinds" of memory. It provides a
low level interface for generating inputs to mbind() and mmap(), and a
high level interface for heap management.  The heap management is
implemented with an extension to the jemalloc library which dedicates
"arenas" to each CPU and kind of memory.  Additionally the heap is
partitioned so that freed memory segments of different kinds are not
coalesced.  The memkind library enables page size selection at
allocationt time.  To use memkind, jemalloc must be compiled with the
--enable-memkind and --with-jemalloc-prefix=je_ options.

%build
if [ ! -f VERSION ]; then
    ./autogen.sh
fi
%if %{defined suse_version}
./configure --prefix=%{_prefix} --libdir=%{_libdir} \
    --includedir=%{_includedir} --sbindir=%{_sbindir} \
    --mandir=%{_mandir} --docdir=%{_docdir}/memkind
%else
./configure --prefix=%{_prefix} --libdir=%{_libdir} \
    --includedir=%{_includedir} --sbindir=%{_sbindir} \
    --mandir=%{_mandir} --docdir=%{_docdir}/memkind-%{version}
%endif
$(make_prefix)%{__make} $(make_postfix)

%install
%{__make} DESTDIR=%{buildroot} install
%{__install} -d %{buildroot}/%{_initddir}
%{__install} init.d/memkind %{buildroot}/%{_initddir}/memkind
rm -f %{buildroot}/%{_libdir}/libmemkind.a
rm -f %{buildroot}/%{_libdir}/libmemkind.la
$(extra_install)

%clean

%post devel
/sbin/ldconfig
if [ -x /sbin/chkconfig ]; then
    /sbin/chkconfig --add memkind
elif [ -x /usr/lib/lsb/install_initd ]; then
    /usr/lib/lsb/install_initd %{_initddir}/memkind
else
    for i in 3 4 5; do
        ln -sf %{_initddir}/memkind /etc/rc.d/rc${i}.d/S90memkind
    done
    for i in 0 1 2 6; do
        ln -sf %{_initddir}/memkind /etc/rc.d/rc${i}.d/K10memkind
    done
fi
%{_initddir}/memkind force-reload >/dev/null 2>&1 || :

%preun devel
if [ -z "$1" ] || [ "$1" == 0 ]; then
    %{_initddir}/memkind stop >/dev/null 2>&1 || :
    if [ -x /sbin/chkconfig ]; then
        /sbin/chkconfig --del memkind
    elif [ -x /usr/lib/lsb/remove_initd ]; then
        /usr/lib/lsb/remove_initd %{_initddir}/memkind
    else
        rm -f /etc/rc.d/rc?.d/???memkind
    fi
fi

%postun devel
/sbin/ldconfig

%files devel
%defattr(-,root,root,-)
%{_includedir}/memkind.h
%{_includedir}/hbwmalloc.h
%{_includedir}/memkind_arena.h
%{_includedir}/memkind_default.h
%{_includedir}/memkind_gbtlb.h
%{_includedir}/memkind_hbw.h
%{_includedir}/memkind_hugetlb.h
%{_libdir}/libmemkind.so.0.0.0
%{_libdir}/libmemkind.so.0
%{_libdir}/libmemkind.so
%{_sbindir}/memkind-pmtt
%{_initddir}/memkind
%if %{defined suse_version}
%dir %{_docdir}/memkind
%doc %{_docdir}/memkind/README
%doc %{_docdir}/memkind/COPYING
%doc %{_docdir}/memkind/VERSION
%else
%dir %{_docdir}/memkind-%{version}
%doc %{_docdir}/memkind-%{version}/README
%doc %{_docdir}/memkind-%{version}/COPYING
%doc %{_docdir}/memkind-%{version}/VERSION
%endif
%doc %{_mandir}/man3/hbwmalloc.3.gz
%doc %{_mandir}/man3/memkind.3.gz
%doc %{_mandir}/man3/memkind_arena.3.gz
%doc %{_mandir}/man3/memkind_default.3.gz
%doc %{_mandir}/man3/memkind_gbtlb.3.gz
%doc %{_mandir}/man3/memkind_hbw.3.gz
%doc %{_mandir}/man3/memkind_hugetlb.3.gz
$(extra_files)

%changelog
* Thu Oct 23 2014 Christopher Cantalupo <christopher.m.cantalupo@intel.com> v0.1
- Now building with autotools.
- Updated documentation.
- Fixed typo in copyright.
- Fixed test scripts to properly handle return code of each test.
- Added C++03 standard allocator that uses hbw_malloc and hbw_free.
* Tue Sep 30 2014 Christopher Cantalupo <christopher.m.cantalupo@intel.com> v0.0.8
- Added GBTLB functionality, code clean up, documentation updates,
  examples directory.  Examples includes stream modified to use
  memkind interface.  Code coverage still lacking, and documentation
  incomplete.

endef

export memkind_spec

