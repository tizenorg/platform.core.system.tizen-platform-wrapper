Name:           tizen-platform-wrapper
Version:        1.0
Release:        0
License:        LGPL-2.0
Summary:        A toolkit to generate the libtizen-platform-config library
Url:            http://www.tizen.org
Group:          Development/Tools
Source:         %{name}-%{version}.tar.bz2
Source1001:     %{name}.manifest
Requires:       gperf

%description
A toolkit to generate the libtizen-platform-config library in tizen-platform-config.

%prep
%setup -q
cp %{SOURCE1001} .

%build
%reconfigure
%__make %{?_smp_mflags}

%install
%make_install

%files
%manifest %{name}.manifest
%license LGPL_2.0
%{_bindir}/*
%{_datadir}/%{name}/*
%{_libdir}/pkgconfig/tizen-platform-wrapper.pc

