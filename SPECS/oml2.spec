%define name		oml2
%define version		2.9.0
%define pkgver		%{version}
%define redmineid	792
%define liboml2cur	9
%define liboml2rev	0
%define liboml2age	0
%define libocommcur	1
%define libocommrev	0
%define libocommage	0

# RPM 4.7 on our Jenkins instance does not seem to do this by default.
%define __spec_install_post /usr/lib/rpm/brp-compress
# Avoid error message related to missing debug files:
# Processing files: oml2-debuginfo-2.9.0rpmtest-1.1.x86_64
# error: Could not open %files file /home/abuild/rpmbuild/BUILD/oml2-2.9.0rpmtest/debugfiles.list: No such file or directory
%define debug_package %{nil}

BuildRoot:              %{_tmppath}/%{name}-%{version}-build
Summary:                OML: The Orbit Measurement Library
License:                MIT
URL:                    http://oml.mytestbed.net
Name:                   %{name}
Version:                %{version}
Release:                1
Source:			http://mytestbed.net/attachments/download/%{redmineid}/oml2-%{version}.tar.gz
Source1:		init.d-oml2-server
Source2:		oml2-server.service
Packager:               Christoph Dwertmann <christoph.dwertmann@nicta.com.au>
Prefix:                 /usr
Group:                  System/Libraries
Conflicts:              liboml liboml-dev oml-server
Requires:		libxml2
Requires:		popt
Requires:		sqlite
BuildRequires:		asciidoc
BuildRequires:		make
BuildRequires:		libxml2-devel
BuildRequires:		popt-devel
BuildRequires:		sqlite-devel
BuildRequires:		texinfo

%description
This library allows application writers to define customizable
measurement points in their applications to record measurements
either to file, or to an OML server over the network.

The package also contains the OML server, proxy server and
proxy server control script.

%package devel
Summary:	liboml2 development headers
Group:		System/Libraries
Provides:	oml2-devel
Requires:	ruby

%description devel
This package contains necessary header files for liboml2 development.

%prep
%setup -q

%build
./configure --prefix=%{_prefix} --sbindir=%{_sbindir} --mandir=%{_mandir} --libdir=%{_libdir} --sysconfdir=%{_sysconfdir} --enable-doc --disable-doxygen-doc --without-python --localstatedir=/var/lib
make %{?_smp_mflags}

%install
make DESTDIR=%{buildroot} install-strip
mkdir -p $RPM_BUILD_ROOT/etc/rc.d/init.d
cp $RPM_SOURCE_DIR/init.d-oml2-server $RPM_BUILD_ROOT/etc/rc.d/init.d/oml2-server
mkdir -p $RPM_BUILD_ROOT/usr/lib/systemd/system
cp $RPM_SOURCE_DIR/oml2-server.service $RPM_BUILD_ROOT/usr/lib/systemd/system
rm -f %{buildroot}/usr/share/info/dir

%clean
rm -rf %{buildroot}

%post
/sbin/chkconfig --add oml2-server
/sbin/chkconfig oml2-server on

%preun
if [ "$1" = 0 ]; then
   /sbin/chkconfig oml2-server off
   /etc/init.d/oml2-server stop
   /sbin/chkconfig --del oml2-server
fi
exit 0

%postun
if [ "$1" -ge 1 ]; then
    /etc/init.d/oml2-server restart
fi
exit 0

%files
%defattr(-,root,root,-)
%{_prefix}/bin/oml2-proxycon
%{_prefix}/bin/oml2-proxy-server
%{_prefix}/bin/oml2-server
%{_libdir}/libocomm.a
%{_libdir}/libocomm.la
%{_libdir}/libocomm.so.%{libocommcur}
%{_libdir}/libocomm.so.%{libocommcur}.%{libocommrev}.%{libocommage}
%{_libdir}/liboml2.a
%{_libdir}/liboml2.la
%{_libdir}/liboml2.so.%{liboml2cur}
%{_libdir}/liboml2.so.%{liboml2cur}.%{liboml2rev}.%{liboml2age}
%{_prefix}/share/oml2/oml2-server-hook.sh
%{_prefix}/share/info/oml-user-manual.info.gz
%doc %{_mandir}/man1/liboml2.1.gz
%doc %{_mandir}/man1/oml2-proxy-server.1.gz
%doc %{_mandir}/man1/oml2-server.1.gz
%doc %{_mandir}/man5
%dir /var/lib/oml2
%attr(0755,root,root) /etc/rc.d/init.d/oml2-server
/usr/lib/systemd/system/oml2-server.service

%files devel
%{_libdir}/liboml2.so
%{_libdir}/libocomm.so
%{_prefix}/include
%{_prefix}/bin/oml2_scaffold
%{_prefix}/bin/oml2-scaffold
%{_prefix}/share/oml2/liboml2/Makefile
%{_prefix}/share/oml2/liboml2/config.h
%{_prefix}/share/oml2/liboml2/README
%{_prefix}/share/oml2/liboml2/generator.rb
%{_prefix}/share/oml2/liboml2/generator.c
%{_prefix}/share/oml2/liboml2-conf/config.xml
%{_prefix}/share/oml2/liboml2-conf/config_two_streams.xml
%{_prefix}/share/oml2/liboml2-conf/config_text_stream.xml
%doc %{_mandir}/man1/oml2-scaffold.1.gz
%doc %{_mandir}/man1/oml2_scaffold.1.gz
%doc %{_mandir}/man3/liboml2.3.gz
%doc %{_mandir}/man3/omlc_add_mp.3.gz
%doc %{_mandir}/man3/omlc_close.3.gz
%doc %{_mandir}/man3/omlc_copy_blob.3.gz
%doc %{_mandir}/man3/omlc_copy_string.3.gz
%doc %{_mandir}/man3/omlc_init.3.gz
%doc %{_mandir}/man3/omlc_inject.3.gz
%doc %{_mandir}/man3/omlc_reset_blob.3.gz
%doc %{_mandir}/man3/omlc_reset_string.3.gz
%doc %{_mandir}/man3/omlc_set_blob.3.gz
%doc %{_mandir}/man3/omlc_set_double.3.gz
%doc %{_mandir}/man3/omlc_set_int32.3.gz
%doc %{_mandir}/man3/omlc_set_int64.3.gz
%doc %{_mandir}/man3/omlc_set_string.3.gz
%doc %{_mandir}/man3/omlc_set_string_copy.3.gz
%doc %{_mandir}/man3/omlc_set_uint32.3.gz
%doc %{_mandir}/man3/omlc_set_uint64.3.gz
%doc %{_mandir}/man3/omlc_start.3.gz
%doc %{_mandir}/man3/omlc_zero.3.gz
%doc %{_mandir}/man3/omlc_zero_array.3.gz
%doc %{_mandir}/man3/oml_inject_MPNAME.3.gz
%doc %{_mandir}/man3/OmlValueU.3.gz
