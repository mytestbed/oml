#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.6])
AC_INIT([oml2], [2.3.0], [http://www.mytestbed.net/projects/oml/issues/new])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])
AC_PROG_LIBTOOL([1.4])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET

# Checks for libraries.

# Check that libxml2 is installed, and work out how to compile/link against it
AC_CHECK_LIB([xml2], [xmlParseFile], [AC_DEFINE([HAVE_LIBXML2], [1], [Define if libxml2 is installed.])])
AC_PATH_PROG([HAVE_XML2CFG], [xml2-config])

if test "x$HAVE_XML2CFG" != "x"; then
   xxCFLAGS=`$HAVE_XML2CFG --cflags`
   xxLIBS=`$HAVE_XML2CFG --libs`
   AC_SUBST([LIBXML2_CFLAGS], [$xxCFLAGS])
   AC_SUBST([LIBXML2_LIBS], [$xxLIBS])
   AC_MSG_CHECKING([how to include libxml2 headers])
   AC_MSG_RESULT([$LIBXML2_CFLAGS])
   AC_MSG_CHECKING([how to link with libxml2])
   AC_MSG_RESULT([$LIBXML2_LIBS])
fi

AM_CONDITIONAL([DARWIN], [test x`uname` = xDarwin])

PKG_CHECK_MODULES([CHECK], [check >= 0.9.4])

AC_CHECK_LIB(sqlite3, sqlite3_initialize, [AC_DEFINE([HAVE_LIBSQLITE3], [1], [Define if libsqlite3 is installed.])])
AC_CHECK_LIB([pcap], [pcap_open_live], [AC_DEFINE([HAVE_LIBPCAP], [1], [Define if libpcap is installed.])])
AC_CHECK_LIB([popt], [poptGetContext], [AC_DEFINE([HAVE_LIBPOPT], [1], [Define if libpopt is installed.])])
AC_CHECK_LIB([pthread], [pthread_create], [AC_DEFINE([HAVE_LIBPTHREAD], [1], [Define if libpthread is installed.])])

# FIXME: Replace `main' with a function in `-lm':
#AC_CHECK_LIB([m], [main])
# FIXME: Replace `main' with a function in `-ltrace':
#AC_CHECK_LIB([trace], [main])

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h malloc.h netdb.h netinet/in.h stdlib.h string.h strings.h sys/ioctl.h sys/socket.h sys/time.h sys/timeb.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_CHECK_FUNCS([gethostbyname gettimeofday inet_ntoa memmove memset socket sqrt strerror])

AC_C_BIGENDIAN

AC_CONFIG_FILES([Makefile
		 lib/Makefile
		 lib/client/Makefile
		 lib/client/tests/Makefile
		 lib/shared/Makefile
		 lib/ocomm/Makefile
		 server/Makefile
		 proxy_server/Makefile
		 app/Makefile
		 app/simple/Makefile
		 app/omf_nmetrics/Makefile
		 app/omf_trace/Makefile
		 app/iperf/Makefile
		 app/gps/Makefile
		 app/wlanconfig/Makefile
		 app/otg/Makefile
		 app/otg/src/cpp/core/Makefile
		 app/otg/src/cpp/port/Makefile
		 app/otg/src/cpp/generator/Makefile
		 ])

#AC_CONFIG_SUBDIRS([libsigar])
AC_OUTPUT
