%define name		oml2
%define srcver	2.11.0
%define pkgver	%{srcver}
%define pkgvernotilde	%{srcver}
%define redmineid	1104

# RPM 4.7 on our Jenkins instance does not seem to do this by default.
%define __spec_install_post /usr/lib/rpm/brp-compress
# Avoid error message related to missing debug files:
# Processing files: oml2-debuginfo-2.9.0rpmtest-1.1.x86_64
# error: Could not open %files file /home/abuild/rpmbuild/BUILD/oml2-2.9.0rpmtest/debugfiles.list: No such file or directory
%define debug_package %{nil}

BuildRoot:      %{_tmppath}/%{name}-%{srcver}-build
Summary:        OML: The full O? Measurement Library and tools (transitional metapackage)
License:        MIT
URL:            http://oml.mytestbed.net
Name:           %{name}
# Use a specific version number if need be (e.g., ~rc instead of rc for proper
# version ordering)
%if 0%{?fedora} < 18 || 0%{?redhat} > 0 || 0%{?centos} > 0
# RPM on Fedora <= 17 doesn't support ~ in version numbers
Version:        %{pkgvernotilde}
%else
Version:        %{pkgver}
%endif
Release:	1
Source:		http://mytestbed.net/attachments/download/%{redmineid}/oml2-%{srcver}.tar.gz
Source1:	init.d-oml2-server
Source2:	oml2-server.service
Packager:       OML developers <oml-user@mytestbed.net>
Prefix:         /usr
Group:          System/Libraries
Requires:	liboml2
Requires:	oml2-server
BuildRequires:	asciidoc
BuildRequires:	make
BuildRequires:	libxml2-devel
BuildRequires:	popt-devel
BuildRequires:	sqlite-devel
BuildRequires:	postgresql-devel
BuildRequires:	ruby
BuildRequires:	texinfo
%if 0%{?fedora} >= 19
BuildRequires:  rubypick
%endif

%description
This is a transitional metapackage installing the liboml2 and oml2-server
packages.

%package devel
Summary:	OML library (liboml2) development headers (transitional metapackage)
Group:		Development/Libraries
Requires:	oml2
Requires:	liboml2-devel

%description devel
This is a transitional metapackage installing the liboml2-devel package.

#
# The real packages start here
#
%package -n liboml2
Summary:        OML: The O? Measurement Library and client tools
Group:          System/Libraries
Provides:	libocomm
Obsoletes:      liboml
Requires:	popt
Requires:	libxml2

%description -n liboml2
This library allows application writers to define customizable
measurement points in their applications to record measurements
either to file, or to an OML server over the network.

The package also contains and example sine generator and the OML proxy
server and its control script.


%package -n liboml2-devel
Summary:	OML library (liboml2) development headers
Group:		Development/Libraries
Provides:	libocomm-devel
Obsoletes:      liboml-dev
Requires:	liboml2
Requires:	popt-devel
Requires:	libxml2-devel
Requires:	ruby
BuildRequires:	sqlite-devel
BuildRequires:	postgresql-devel

%description -n liboml2-devel
This package contains necessary header files for liboml2 development.


%package server
Summary:        OML's measurements collection server
Group:		System/Daemons
Provides:	oml2-server oml-server
Obsoletes:	oml-server
Requires:	liboml2
Requires:	popt
Requires:	sqlite
Requires:	postgresql-libs

%description server
The OML server accepts connections from client applications and stores
measurements that they transmit in a database.  Currently the SQLite3
database is supported.  The server understands both binary and text
protocols on the same TCP port.


%prep
%setup -q -n %{name}-%{srcver}

%build
%configure --enable-doc --disable-doxygen-doc --with-pgsql %{?pgsqlinc:--with-pgsql-inc=%{pgsqlinc}} --without-python --localstatedir=/var/lib
make %{?_smp_mflags}

%install
make DESTDIR=%{buildroot} install-strip
make -C example/liboml2 \
	     SCAFFOLD="%{buildroot}%{_prefix}/bin/oml2-scaffold" \
	     BINDIR="%{buildroot}%{_prefix}/bin" \
	     CFLAGS="-I%{buildroot}%{_prefix}/include" \
	     LDFLAGS="-L%{buildroot}%{_libdir}" \
	     LIBS="-loml2 -locomm -lpopt -lm" \
	     install
mv %{buildroot}%{_prefix}/bin/generator %{buildroot}%{_prefix}/bin/oml2-generator
mkdir -p $RPM_BUILD_ROOT/etc/rc.d/init.d
cp $RPM_SOURCE_DIR/init.d-oml2-server $RPM_BUILD_ROOT/etc/rc.d/init.d/oml2-server
mkdir -p $RPM_BUILD_ROOT%{_prefix}/lib/systemd/system
cp $RPM_SOURCE_DIR/oml2-server.service $RPM_BUILD_ROOT%{_prefix}/lib/systemd/system
rm -f %{buildroot}%{_prefix}/share/info/dir
echo "oml2 metapackage installed" > %{buildroot}%{_prefix}/share/oml2/oml2.meta
echo "oml2-devel metapackage installed" > %{buildroot}%{_prefix}/share/oml2/oml2-devel.meta

%clean
rm -rf %{buildroot}

%post server
/sbin/chkconfig --add oml2-server
/sbin/chkconfig oml2-server on

%preun server
if [ "$1" = 0 ]; then
   /sbin/chkconfig oml2-server off
   /bin/systemctl stop oml2-server.service
   /sbin/chkconfig --del oml2-server
fi
exit 0

%postun server
if [ "$1" -ge 1 ]; then
   /bin/systemctl restart oml2-server.service
fi
exit 0

%files
%defattr(-,root,root,-)
%{_prefix}/share/oml2/oml2.meta

%files devel
%defattr(-,root,root,-)
%{_prefix}/share/oml2/oml2-devel.meta

%files -n liboml2
%defattr(-,root,root,-)
%{_prefix}/bin/oml2-generator
%{_prefix}/bin/oml2-proxycon
%{_prefix}/bin/oml2-proxy-server
%{_libdir}/libocomm.a
%{_libdir}/libocomm.la
%{_libdir}/libocomm.so.*
%{_libdir}/liboml2.a
%{_libdir}/liboml2.la
%{_libdir}/liboml2.so.*
%{_prefix}/share/info/oml-user-manual.info.gz
%doc %{_mandir}/man1/liboml2.1.gz
%doc %{_mandir}/man1/oml2-proxy-server.1.gz
%doc %{_mandir}/man1/oml2-proxycon.1.gz
%doc %{_mandir}/man5

%files -n liboml2-devel
%defattr(-,root,root,-)
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
%doc %{_mandir}/man3/omlc_guid_generate.3.gz
%doc %{_mandir}/man3/omlc_init.3.gz
%doc %{_mandir}/man3/omlc_inject.3.gz
%doc %{_mandir}/man3/omlc_inject_metadata.3.gz
%doc %{_mandir}/man3/omlc_reset_blob.3.gz
%doc %{_mandir}/man3/omlc_reset_string.3.gz
%doc %{_mandir}/man3/omlc_set_blob.3.gz
%doc %{_mandir}/man3/omlc_set_double.3.gz
%doc %{_mandir}/man3/omlc_set_guid.3.gz
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
%doc %{_mandir}/man3/oml_register_mps.3.gz
%doc %{_mandir}/man3/OmlValueU.3.gz

%files server
%defattr(-,root,root,-)
%doc %{_mandir}/man1/oml2-server.1.gz
%dir /var/lib/oml2
%{_prefix}/bin/oml2-server
%{_prefix}/share/oml2/server/oml2-server-hook.sh
%{_prefix}/share/oml2/server/haproxy.conf
%{_prefix}/share/oml2/oml2-server.rb
%attr(0755,root,root) /etc/rc.d/init.d/oml2-server
/usr/lib/systemd/system/oml2-server.service
