%define name		oml2
%define version		2.9.0rc
%define pkgver		%{version}
%define redmineid	767
%define liboml2cur	9
%define liboml2rev	0
%define liboml2age	0
%define libocommcur	1
%define libocommrev	0
%define libocommage	0

# RPM 4.7 on our Jenkins instance does not seem to do this by default.
%define __spec_install_post /usr/lib/rpm/brp-compress

BuildRoot:              %{_tmppath}/%{name}-%{version}-build
Summary:                OML: The Orbit Measurement Library
License:                MIT
URL:                    http://oml.mytestbed.net
Name:                   %{name}
Version:                %{version}
Release:                1
Source:			http://mytestbed.net/attachments/download/%{redmineid}/oml2-%{version}.tar.gz
Packager:               Christoph Dwertmann <christoph.dwertmann@nicta.com.au>
Prefix:                 /usr
Group:                  System/Libraries
Conflicts:              liboml liboml-dev oml-server
Requires:		libxml2 popt sqlite
BuildRequires:          make libxml2-devel popt-devel sqlite-devel texinfo asciidoc

%description
This library allows application writers to define customizable
measurement points in their applications to record measurements
either to file, or to an OML server over the network.

The package also contains the OML server, proxy server and
proxy server control script.

%package devel
Summary: 	liboml2 development headers
Group: 		System/Libraries
Provides: 	oml2-devel

%description devel
This package contains necessary header files for liboml2 development.

%prep
%setup -q

%build
./configure --prefix /usr --enable-doc --disable-doxygen-doc --without-python --localstatedir=/var/lib
make %{?_smp_mflags}

%install
make DESTDIR=%{buildroot} install-strip
rm -f %{buildroot}/usr/share/info/dir

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root,-)
/usr/bin/oml2-proxycon
/usr/bin/oml2-proxy-server
/usr/bin/oml2-server
/usr/lib/libocomm.a
/usr/lib/libocomm.la
/usr/lib/libocomm.so.%{libocommcur}
/usr/lib/libocomm.so.%{libocommcur}.%{libocommrev}.%{libocommage}
/usr/lib/liboml2.a
/usr/lib/liboml2.la
/usr/lib/liboml2.so.%{liboml2cur}
/usr/lib/liboml2.so.%{liboml2cur}.%{liboml2rev}.%{liboml2age}
/usr/share/oml2/oml2-server-hook.sh
/usr/share/info/oml-user-manual.info.gz
%doc /usr/share/man/man1/liboml2.1.gz
%doc /usr/share/man/man1/oml2-proxy-server.1.gz
%doc /usr/share/man/man1/oml2-server.1.gz
%doc /usr/share/man/man5
%dir /var/lib/oml2

%files devel
/usr/lib/liboml2.so
/usr/lib/libocomm.so
/usr/include
/usr/bin/oml2_scaffold
/usr/bin/oml2-scaffold
/usr/share/oml2/liboml2/Makefile
/usr/share/oml2/liboml2/version.h
/usr/share/oml2/liboml2/README
/usr/share/oml2/liboml2/generator.rb
/usr/share/oml2/liboml2/generator.c
/usr/share/oml2/liboml2-conf/config.xml
/usr/share/oml2/liboml2-conf/config_two_streams.xml
/usr/share/oml2/liboml2-conf/config_text_stream.xml
#/usr/share/oml2/oml4r/oml4r-wlanconfig.rb
#/usr/share/oml2/oml4r/oml4r-simple-example.rb
#/usr/share/oml2/oml4r/oml4r-%{version}.dirty.gem
%doc /usr/share/man/man1/oml2-scaffold.1.gz
%doc /usr/share/man/man1/oml2_scaffold.1.gz
%doc /usr/share/man/man3/liboml2.3.gz
%doc /usr/share/man/man3/omlc_add_mp.3.gz
%doc /usr/share/man/man3/omlc_close.3.gz
%doc /usr/share/man/man3/omlc_copy_blob.3.gz
%doc /usr/share/man/man3/omlc_copy_string.3.gz
%doc /usr/share/man/man3/omlc_init.3.gz
%doc /usr/share/man/man3/omlc_inject.3.gz
%doc /usr/share/man/man3/omlc_reset_blob.3.gz
%doc /usr/share/man/man3/omlc_reset_string.3.gz
%doc /usr/share/man/man3/omlc_set_blob.3.gz
%doc /usr/share/man/man3/omlc_set_double.3.gz
%doc /usr/share/man/man3/omlc_set_int32.3.gz
%doc /usr/share/man/man3/omlc_set_int64.3.gz
%doc /usr/share/man/man3/omlc_set_string.3.gz
%doc /usr/share/man/man3/omlc_set_string_copy.3.gz
%doc /usr/share/man/man3/omlc_set_uint32.3.gz
%doc /usr/share/man/man3/omlc_set_uint64.3.gz
%doc /usr/share/man/man3/omlc_start.3.gz
%doc /usr/share/man/man3/omlc_zero.3.gz
%doc /usr/share/man/man3/omlc_zero_array.3.gz
%doc /usr/share/man/man3/oml_inject_MPNAME.3.gz
%doc /usr/share/man/man3/OmlValueU.3.gz
