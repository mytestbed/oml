#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.6])
AC_INIT([oml2], [2.6.1], [http://omf.mytestbed.net/wiki/oml/FAQ_and_Support])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])
AC_PROG_LIBTOOL([1.4])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET

missing_check=""
PKG_CHECK_MODULES([CHECK], [check >= 0.9.4], [], [missing_check="yes"])
AM_CONDITIONAL([HAVE_CHECK], [test x"${missing_check}" != xyes])

# Checks for libraries.
missing_libs=""

AC_SUBST([OML_VER_MAJOR], [`echo "$PACKAGE_VERSION" | awk -F. '{print $1}'`])
AC_SUBST([OML_VER_MINOR], [`echo "$PACKAGE_VERSION" | awk -F. '{print $2}'`])
AC_SUBST([OML_VER_PATCH], [`echo "$PACKAGE_VERSION" | awk -F. '{print $3}'`])
AC_SUBST([OML_VER_PATCH_LT], [`echo "$PACKAGE_VERSION" | awk -F. '{print $3}' | sed -ne 's/[[^0-9]]*\([[0-9]]*\)/\1/p'`])

AC_SUBST([pkglocalstatedir], [${localstatedir}/${PACKAGE}])

# Check that libxml2 is installed, and work out how to compile/link against it
AC_CHECK_LIB([xml2], [xmlParseFile], [AC_DEFINE([HAVE_LIBXML2], [1], [Define if libxml2 is installed.])], [missing_libs+=" libxml2"])
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

# Check for presence of other libraries.
AC_CHECK_LIB([sqlite3], [sqlite3_open], [AC_DEFINE([HAVE_LIBSQLITE3], [1], [Define if libsqlite3 is installed.])], [missing_libs+=" libsqlite3"])
AC_CHECK_LIB([popt], [poptGetContext], [AC_DEFINE([HAVE_LIBPOPT], [1], [Define if libpopt is installed.])], [missing_libs+=" libpopt"])
AC_CHECK_LIB([pthread], [pthread_create], [AC_DEFINE([HAVE_LIBPTHREAD], [1], [Define if libpthread is installed.])], [missing_libs+=" libpthread"])
AC_CHECK_LIB([m], [sqrt], [AC_DEFINE([HAVE_LIBM], [1], [Define if libm is installed (It should be!)])], [missing_libs+=" libm"])

# Check if the user asked to build with PostgreSQL support enabled.
AC_ARG_WITH([postgresql],
		[AS_HELP_STRING([--with-postgresql],
		  [support oml2-server storing measurements in a PostgreSQL database @<:@default=no@:>@])],
		[],
		[with_postgresql=no])

if test "x$with_postgresql" != "xno"; then
   # Check that the PostgreSQL development libraries are installed, and work out how to compile/link against them.
   AC_PATH_PROG([HAVE_PGCFG], [pg_config])
   AM_CONDITIONAL([HAVE_PG], [test "x${HAVE_PGCFG}" != "x"])

   if test "x$HAVE_PGCFG" != "x"; then
      AC_DEFINE([HAVE_PG], [1], [Define if libpq is installed])
	  xxCFLAGS=-I`$HAVE_PGCFG --includedir`
	  xxLDFLAGS=-L`$HAVE_PGCFG --libdir`
	  AC_SUBST([LIBPQ_CFLAGS], [$xxCFLAGS])
	  AC_SUBST([LIBPQ_LDFLAGS], [$xxLDFLAGS])
	  AC_SUBST([LIBPQ_LIBS], [-lpq])
	  AC_MSG_CHECKING([how to include PostgreSQL headers])
	  AC_MSG_RESULT([$LIBPQ_CFLAGS])
	  AC_MSG_CHECKING([location of libpq])
	  AC_MSG_RESULT([$LIBPQ_LDFLAGS])
   else
	  missing_libs+=" libpq"
   fi
else
   AC_DEFINE([HAVE_PG], [0], [Define if libpq is installed])
   AM_CONDITIONAL([HAVE_PG], [false])
fi


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

AC_ARG_WITH([doc],
		[AS_HELP_STRING([--with-doc],
		  [build documentation (man pages, manual) @<:@default=no@:>@])],
		[],
		[with_doc=no])

if test "x$with_doc" != "xno"; then
   # Check for asciidoc (for generating manpages).
   AC_PATH_PROG(A2X, a2x, no)
   AC_PATH_PROG(ASCIIDOC, asciidoc, no)
   AM_CONDITIONAL([HAVE_A2X], [test "x$A2X" != "xno"])
   AM_CONDITIONAL([HAVE_ASCIIDOC], [test "x$ASCIIDOC" != "xno"])
   if test x"$A2X" = x"no"; then
      AC_MSG_WARN([Test for a2x failed.  Not generating man pages (but OML build should still complete ok).])
   fi
   if test x"$ASCIIDOC" = x"no"; then
      AC_MSG_WARN([Test for asciidoc failed.  Not generating man pages (but OML build should still complete ok).])
   fi
else
   AM_CONDITIONAL([HAVE_A2X], [false])
   AM_CONDITIONAL([HAVE_ASCIIDOC], [false])
fi

AC_CONFIG_FILES([Makefile
		 lib/Makefile
		 lib/client/Makefile
		 lib/shared/Makefile
		 lib/ocomm/Makefile
		 server/Makefile
		 proxy_server/Makefile
		 doc/Makefile
		 test/Makefile
		 test/lib/Makefile
		 test/server/Makefile
		 test/system/blob/Makefile
		 ])

if test "x$missing_check" != "x"; then
   AC_MSG_WARN([could not find the `check' package, required
for running unit tests. This will not stop OML from building,
but it will cause `make check' to do nothing. Install the `check'
package (http://check.sourceforge.net) if you wish to run the
unit tests.])
fi

# Let the user know we're missing required libraries, and don't generate the build system.
if test "x$missing_libs" != "x"; then
   for lib in $missing_libs; do
      AC_MSG_WARN([could not find required library $lib])
   done
   AC_MSG_ERROR([missing libraries, could not continue.
Please install all of the following libraries and then re-run configure
(you might need the development header packages (-dev) as well if you are
using your system's package manager to install them):
$missing_libs])
fi

AC_OUTPUT
