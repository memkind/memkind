#
#  Copyright (2014) Intel Corporation All Rights Reserved.
#
#  This software is supplied under the terms of a license
#  agreement or nondisclosure agreement with Intel Corp.
#  and may not be copied or disclosed except in accordance
#  with the terms of that agreement.
#

define make_spec

Summary: Extention to libnuma for kinds of memory
Name: memkind
Version: $(version)
Release: $(release)
License: See COPYING
Group: System Environment/Libraries
Vendor: Intel Corporation
URL: http://www.intel.com
Source0: memkind-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root
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
"arenas" to each CPU and kind of memory.  Additionally the heap
is partitioned so that freed memory segments of different kinds are
not coalesced.  To use memkind, jemalloc must be compiled with the
--enable-memkind option.

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
"arenas" to each CPU and kind of memory.  Additionally the heap
is partitioned so that freed memory segments of different kinds are
not coalesced.  To use memkind, jemalloc must be compiled with the
--enable-memkind option.

%build
$(make_prefix) $(MAKE) $(make_postfix)

%install
make DESTDIR=%{buildroot} VERSION=%{version} includedir=%{_includedir} libdir=%{_libdir} sbindir=%{_sbindir} initddir=%{_initddir} docdir=%{_docdir} mandir=%{_mandir} install
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
%{_libdir}/libmemkind.so.0.0
%{_libdir}/libmemkind.so.0
%{_libdir}/libmemkind.so
%{_sbindir}/memkind-pmtt
%{_initddir}/memkind
%dir %{_docdir}/memkind-%{version}
%doc %{_docdir}/memkind-%{version}/README.txt
%doc %{_docdir}/memkind-%{version}/COPYING.txt
%doc %{_mandir}/man3/hbwmalloc.3.gz
%doc %{_mandir}/man3/memkind.3.gz
%doc %{_mandir}/man3/memkind_arena.3.gz
%doc %{_mandir}/man3/memkind_default.3.gz
%doc %{_mandir}/man3/memkind_gbtlb.3.gz
%doc %{_mandir}/man3/memkind_hbw.3.gz
%doc %{_mandir}/man3/memkind_hugetlb.3.gz
$(extra_files)

%changelog
* Tue Jul 1 2014 Christopher Cantalupo <christopher.m.cantalupo@intel.com> -
- Initial release to NDA customers
endef

export make_spec

