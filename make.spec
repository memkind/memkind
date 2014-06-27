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
/sbin/chkconfig --add numakind
/sbin/service numakind force-reload >/dev/null 2>&1

%preun
if [ -z "$1" ] || [ "$1" == 0 ]
then
    /sbin/service numakind stop >/dev/null 2>&1
    /sbin/chkconfig --del numakind
fi

%postun
/sbin/ldconfig

%files
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
