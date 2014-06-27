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
Name: $(name)
Version: $(version)
Release: $(release)
License: See COPYING
Group: System Environment/Libraries
Vendor: Intel Corporation
URL: http://www.intel.com
Source0: numakind-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root
BuildRequires: numactl
%if %{defined suse_version}
BuildRequires: libnuma-devel
%else
BuildRequires: numactl-devel
%endif
%if ! %{defined jemalloc_installed}
BuildRequires: jemalloc-devel
%endif

%description
The numakind library extends libnuma with the ability to categorize
groups of NUMA nodes into different "kinds" of memory. It provides a
low level interface for generating inputs to mbind() and mmap(), and a
high level interface for heap management.  The heap management is
implemented with an extension to the jemalloc library which dedicates
"arenas" to each CPU node and kind of memory.  Additionally the heap
is partitioned so that freed memory segments of different kinds are
not coalesced.  To use numakind, jemalloc must be compiled with the
--enable-numakind option.

%package devel
Summary: Development pacakge for extention to libnuma for kinds of memory
Group: System Environment/Libraries

%description devel
The numakind library extends libnuma with the ability to categorize
groups of NUMA nodes into different "kinds" of memory. It provides a
low level interface for generating inputs to mbind() and mmap(), and a
high level interface for heap management.  The heap management is
implemented with an extension to the jemalloc library which dedicates
"arenas" to each CPU node and kind of memory.  Additionally the heap
is partitioned so that freed memory segments of different kinds are
not coalesced.  To use numakind, jemalloc must be compiled with the
--enable-numakind option.

%prep
%setup -D -q -c -T -a 0

%build
$(make_prefix) $(MAKE) $(make_postfix)

%install
make DESTDIR=%{buildroot} VERSION=%{version} includedir=%{_includedir} libdir=%{_libdir} sbinddir={%_sbinddir} initddir=%{_initddir} docdir=%{_docdir} mandir=%{_mandir} install
$(extra_install)

%clean

%post
/sbin/ldconfig
if [ -x /usr/lib/lsb/install_initd ]; then
    /usr/lib/lsb/install_initd %{_initddir}/numakind
elif [ -x /sbin/chkconfig ]; then
    /sbin/chkconfig --add numakind
else
    for i in 3 4 5; do
        ln -sf %{_initddir}/numakind /etc/rc.d/rc${i}.d/S90numakind
    done
    for i in 0 1 2 6; do
        ln -sf %{_initddir}/numakind /etc/rc.d/rc${i}.d/K10numakind
    done
fi
%{_initddir}/numakind force-reload >/dev/null 2>&1

%preun
if [ -z "$1" ] || [ "$1" == 0 ]; then
    %{_initdir}/numakind stop >/dev/null 2>&1
    if [ -x /usr/lib/lsb/remove_initd ]; then
        /usr/lib/lsb/remove_initd %{_initdir}/numakind
    elif [ -x /sbin/chkconfig ]; then
        /sbin/chkconfig --del numakind
    else
        rm -f /etc/rc.d/rc?.d/???numakind
    fi
fi

%postun
/sbin/ldconfig

%files devel
%defattr(-,root,root,-)
%{_includedir}/numakind.h
%{_includedir}/hbwmalloc.h
%{_libdir}/libnumakind.so.0.0
%{_libdir}/libnumakind.so.0
%{_libdir}/libnumakind.so
%{_sbindir}/numakind-pmtt
%{_initddir}/numakind
%doc %{_docdir}/numakind-%{version}/README.txt
%doc %{_docdir}/numakind-%{version}/COPYING.txt
%doc %{_mandir}/man3/hbwmalloc.3.gz
%doc %{_mandir}/man3/numakind.3.gz
$(extra_files)

%changelog
* Mon Mar 24 2014 mic <mic@localhost> - 
- Initial build.
endef

export make_spec
