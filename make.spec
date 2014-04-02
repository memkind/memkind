define numakind_specfile

Summary: Extention to libnuma for kinds of memory
Name: $(name)
Version: $(version)
Release: 1
License: See COPYING
Group: System Environment/Libraries
Vendor: Intel Corporation
URL: http://www.intel.com
Source0: %{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root
BuildRequires: jemalloc
BuildRequires: numactl
BuildRequires: numactl-devel

%description
The numakind library extends libnuma with the ability to categorize
groups of numa nodes into different "kinds" of memory. It provides a
low level interface for generating inputs to mbind() and mmap(), and a
high level interface for heap management.  The heap management is
implemented with an extension to the jemalloc library which dedicates
"arenas" to each CPU node and kind of memory.  To use numakind,
jemalloc must be compiled with the --enable-numakind option.

%prep
%setup -q -c -T -a 0

%build
$(make_prefix) $(MAKE)

%install
make DESTDIR=%{buildroot} VERSION=%{version} install

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
%doc /usr/share/doc/numakind-%{version}/README.txt
$(coverage_file)

%changelog
* Mon Mar 24 2014 mic <mic@localhost> - 
- Initial build.
endef

export numakind_specfile
