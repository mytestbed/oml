%define name		oml2
%define release		1
%define version 	%(grep "PACKAGE_VERSION =" Makefile | awk '{print $3;}')
%define buildroot 	%{name}-%{version}-root

BuildRoot:		%{buildroot}
Summary: 		OML: The Orbit Measurement Library
License: 		MIT
Vendor:			National ICT Australia
URL:			http://oml.mytestbed.net
Packager:		Christoph Dwertmann <christoph.dwertmann@nicta.com.au>
Name: 			%{name}
Version: 		%{version}
Release: 		%{release}
Source: 		%{name}-%{version}.tar.gz
Prefix: 		/usr
Group: 			System/Libraries
Conflicts:		liboml liboml-dev oml-server

%description
This library allows application writers to define customizable
measurement points in their applications to record measurements
either to file, or to an OML server over the network.

The package also contains the liboml2 headers, libocomm + headers,
OML server, proxy server and proxy server control script.

%prep
%setup -q

%build
./configure --prefix=$RPM_BUILD_ROOT/usr --with-doc
make

%install
make install prefix=$RPM_BUILD_ROOT/usr

%files
%defattr(-,root,root)
/usr/*
