#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([oml2],
	m4_esyscmd([build-aux/git-version-gen .tarball-version]),
	[oml-user@lists.nicta.com.au])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([-Wall -Werror foreign])
# automake 1.12 seems to require this, but automake 1.11 doesn't recognize it
m4_pattern_allow([AM_PROG_AR]) 
AM_PROG_AR

LT_INIT

AC_PROG_CXX
AC_PROG_CC
gl_EARLY
AM_PROG_CC_C_O
AC_PROG_INSTALL
AC_PROG_MAKE_SET

gl_INIT

OML_CHECK_MACOSX

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

# Checks for libraries.
missing_libs=""

AC_SUBST([OML_VER_MINOR_LT], [`echo "$PACKAGE_VERSION" | awk -F. '{print $2}'`])
AC_SUBST([OML_VER_PATCH_LT], [`echo "$PACKAGE_VERSION" | awk -F. '{print $3$4}' | sed -ne 's/[[^0-9]]*\([[0-9]]*\)[[^0-9]].*/\1/p'`])
AC_SUBST([OML_GEM_VER], [`echo "$PACKAGE_VERSION" | sed s/-/./g`])

AC_SUBST([pkglocalstatedir], [${localstatedir}/${PACKAGE}])

# Check for presence of libraries.
AC_SEARCH_LIBS([sqrt], [m], [AC_DEFINE([HAVE_LIBM], [1], [Define if libm is installed (It should be!)])], [missing_libs+=" libm"])
AC_SEARCH_LIBS([pthread_create], [pthread], [AC_DEFINE([HAVE_LIBPTHREAD], [1], [Define if libpthread is installed.])], [missing_libs+=" libpthread"])
AC_SEARCH_LIBS([poptGetContext], [popt], [AC_DEFINE([HAVE_LIBPOPT], [1], [Define if libpopt is installed.])], [missing_libs+=" libpopt"])

# Check that libxml2 is installed, and work out how to compile/link against it
AC_SEARCH_LIBS([xmlParseFile], [xml2], [AC_DEFINE([HAVE_LIBXML2], [1], [Define if libxml2 is installed.])], [missing_libs+=" libxml2"])
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


AC_SEARCH_LIBS([sqlite3_open], [sqlite3], [AC_DEFINE([HAVE_LIBSQLITE3], [1], [Define if libsqlite3 is installed.])], [missing_libs+=" libsqlite3"])

# Detect if PostgreSQL is around, but don't add it to the default
oldLIBS=$LIBS
AX_CHECK_POSTGRES_DB([],
		     [AS_IF([test "$pg_prefix" != ""],[missing_libs+=" libpq"])])
AM_CONDITIONAL([HAVE_LIBPQ], [test x$HAVE_LIBPQ = xyes])
LIBS=$oldLIBS

AX_WITH_PROG([RUBY],[ruby])
AX_PROG_RUBY_VERSION([1.8.7],
		     [
		      missing_rubyge187=no
		      dnl $ruby_version is defined by AX_PROG_RUBY_VERSION prior to running this
		      AX_COMPARE_VERSION([1.9],[le],[$ruby_version],[missing_rubyge19=no],[
					  AC_MSG_WARN([No Ruby>=1.9 found; some tests will be skipped])
					  missing_rubyge19=yes
					  ])
		      AX_WITH_PROG([GEM],[gem])
		      AX_WITH_PROG([RAKE],[rake])
		      AS_IF([test -z "$GEM"],[
					    AC_MSG_WARN([Ruby found, but not the corresponding gem utility; the OML4r gem will not be built])
					   ],
					   [test -z "$RAKE"],
					   [
					    AC_MSG_WARN([Ruby found, but not the corresponding rake utility; the OML4r gem will not be built])
					   ],
					    dnl Hack to avoid nesting AS_IFs:
					    dnl We actually do the bundler check befor telling the user
					   [ ${GEM} list | grep -q "^bundler\s" ],
					   [
					    AC_MSG_CHECKING(whether the bundler gem is available)
					    have_bundler=yes
					    AC_MSG_RESULT([${have_bundler}])
					   ],
					   [
					    AC_MSG_CHECKING(whether the bundler gem is available)
					    have_bundler=no
					    AC_MSG_RESULT([${have_bundler}])
					    AC_MSG_WARN([The OML4r gem will not be built; you can run \`gem install bundler' and rerun configure to fix this issue])
					   ]
			   )
		     ],
		     [
		      AC_MSG_WARN([No Ruby>=1.8.7 found; the OML4r gem will not be built, and some tests will be skipped])
		      missing_rubyge187=yes
		      missing_rubyge19=yes
		     ]
		    )
AM_CONDITIONAL([HAVE_RUBY_MIN_1_8_7], [test x"${missing_rubyge187}" != xyes])
AM_CONDITIONAL([HAVE_RUBY_MIN_1_9], [test x"${missing_rubyge19}" != xyes])
AM_CONDITIONAL([HAVE_GEM_BUNDLER], [test x$have_bundler = xyes])

missing_check=""
PKG_CHECK_MODULES([CHECK], [check >= 0.9.4], [], [missing_check="yes"])
AM_CONDITIONAL([HAVE_CHECK], [test x"${missing_check}" != xyes])

AC_ARG_ENABLE([doc],
		[AS_HELP_STRING([--disable-doc],
				[do not build documentation (man pages, manual)])],
		[enable_doc=yes])

if test "x$enable_doc" != "xno"; then
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
		 gnulib/Makefile
		 lib/Makefile
		 lib/client/Makefile
		 lib/shared/Makefile
		 lib/ocomm/Makefile
		 ruby/Makefile
		 ruby/oml4r/lib/oml4r.rb
		 server/Makefile
		 proxy_server/Makefile
		 doc/Makefile
		 test/Makefile
		 test/lib/Makefile
		 test/ruby/Makefile
		 test/server/Makefile
		 test/server/issue610.rb
		 test/system/blob/Makefile
		 ])
AC_CONFIG_COMMANDS([issue610-exec], [chmod a+x test/server/issue610.rb])
dnl Copy source files for the Gem in VPATH so rake is happy
AS_IF([test x$srcdir != x. && test x$have_bundler = xyes], 
      [AC_CONFIG_COMMANDS([oml4r-vpath-copy],
			  [mkdir -p ruby/oml4r/lib/oml4r && \
			   for file in \
				   ruby/oml4r/Rakefile \
				   ruby/oml4r/lib/oml4r/oml4r-wlanconfig.rb \
				   ruby/oml4r/lib/oml4r/oml4r-simple-example.rb \
				   ruby/oml4r/README.md \
				   ruby/oml4r/oml4r.gemspec \
				   ruby/oml4r/LICENSE \
				   ruby/oml4r/Gemfile \
				   ruby/oml4r/.yardopts; do
			  cp -f $srcdir/${file} ./${file}; done
			  ],[srcdir=$srcdir])
])
dnl If the above command has copied files, let ruby/Makefile.am know that they have to be cleaned up
AM_CONDITIONAL([CLEANUP_OML4R_GEM_SRC], [test x$srcdir != x. && test x$have_bundler = xyes])

if test "x$missing_check" != "x"; then
   AC_MSG_WARN([could not find the `check' package, required for running unit tests. This will not stop OML from building, but it will cause `make check' to do nothing. Install the `check' package (http://check.sourceforge.net) if you wish to run the unit tests.])
fi

# Let the user know we're missing required libraries, and don't generate the build system.
if test "x$missing_libs" != "x"; then
   for lib in $missing_libs; do
      AC_MSG_WARN([could not find required library $lib])
   done
   AC_MSG_ERROR([missing libraries, could not continue.  Please install all of the following libraries and then re-run configure (you might need the development header packages (-dev) as well if you are using your system's package manager to install them): $missing_libs])
fi

AC_OUTPUT
