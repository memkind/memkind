Summary:   Portable Hardware Locality - portable abstraction of hierarchical architectures
Name:      hwloc
Version:   2.3.0
Release:   0kb1%{?dist}
License:   BSD
URL:       http://www.open-mpi.org/projects/hwloc/
Source0:   http://www.open-mpi.org/software/hwloc/v2.3/downloads/%{name}-%{version}.tar.gz

%description
The Portable Hardware Locality (hwloc) software package provides
a portable abstraction (across OS, versions, architectures, ...)
of the hierarchical topology of modern architectures, including
NUMA memory nodes, shared caches, processor sockets, processor cores
and processing units (logical processors or "threads"). It also gathers
various system attributes such as cache and memory information. It primarily
aims at helping applications with gathering information about modern
computing hardware so as to exploit it accordingly and efficiently.

hwloc may display the topology in multiple convenient formats.
It also offers a powerful programming interface (C API) to gather information
about the hardware, bind processes, and much more.

%package        devel
Summary:        Development libraries for hwloc
Requires:       %{name} = %{version}-%{release}

%description    devel
Development libraries for hwloc

%package        docs
Summary:        Documentation for hwloc
Requires:       %{name} = %{version}-%{release}

%description    docs
Documentation for hwloc

%prep
%autosetup -p1

%build

%configure
make %{?_smp_mflags}

%install
make DESTDIR=%{buildroot} install
find %{buildroot} -name '*.la' -delete

%pre

%post
/sbin/ldconfig

%preun

# Post-uninstall
/sbin/ldconfig

%clean
rm -rf %{buildroot}/*

%files
%defattr(-,root,root)
%{_bindir}/*
%ifarch x86_64
%{_sbindir}/*
%endif
%{_libdir}/*.so.*

%files devel
%defattr(-,root,root)
%{_includedir}/*
%{_libdir}/*.so
%{_libdir}/pkgconfig/*.pc

%files docs
%defattr(-,root,root)
%{_mandir}/*
%{_docdir}/*
%{_datadir}/%{name}/*
%{_datadir}/bash-completion/completions/hwloc
