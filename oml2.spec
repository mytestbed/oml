%define name            oml2
%define version         2.6.1

BuildRoot:              %{_tmppath}/%{name}-%{version}-build
Summary:                OML: The Orbit Measurement Library
License:                MIT
URL:                    http://oml.mytestbed.net
Name:                   %{name}
Version:                %{version}
Release:		1
#Source:                 http://oml.mytestbed.net/attachments/download/583/oml2-2.6.1.tar.gz
Source:			http://pkg.mytestbed.net/test/oml2-2.6.1.tar.gz
Packager:               Christoph Dwertmann <christoph.dwertmann@nicta.com.au>
Prefix:                 /usr
Group:                  System/Libraries
Conflicts:              liboml liboml-dev oml-server
BuildRequires:          autoconf make automake libtool libxml2-devel popt-devel sqlite-devel texinfo asciidoc

%description
This library allows application writers to define customizable
measurement points in their applications to record measurements
either to file, or to an OML server over the network.

The package also contains the liboml2 headers, libocomm + headers,
OML server, proxy server and proxy server control script.

%prep
%setup -q

%build
./configure --prefix=%{buildroot}/usr --with-doc
make %{?_smp_mflags}

%install
make install prefix=%{buildroot}/usr

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root,-)
/usr/*

