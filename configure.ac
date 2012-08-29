#
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_COPYRIGHT([Copyright (C) 2012 NICTA, Olivier Mehani.])
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
AC_SUBST([OML_ALT_VER], [`echo "$PACKAGE_VERSION" | sed s/-/./g`])

AC_SUBST([pkglocalstatedir], [${localstatedir}/${PACKAGE}])

# Check for presence of libraries.
AC_SEARCH_LIBS([sqrt], [m], [AC_DEFINE([HAVE_LIBM], [1], [Define if libm is installed (It should be!)])], [missing_libs+=" libm"])
AC_SEARCH_LIBS([pthread_create], [pthread], [AC_DEFINE([HAVE_LIBPTHREAD], [1], [Define if libpthread is installed.])], [missing_libs+=" libpthread"])
AC_SEARCH_LIBS([poptGetContext], [popt], [AC_DEFINE([HAVE_LIBPOPT], [1], [Define if libpopt is installed.])], [missing_libs+=" libpopt"])

# Check that libxml2 is installed, and work out how to compile/link against it
AC_SEARCH_LIBS([xmlParseFile], [xml2], [AC_DEFINE([HAVE_LIBXML2], [1], [Define if libxml2 is installed.])], [missing_libs+=" libxml2"])
AC_PATH_PROG([HAVE_XML2CFG], [xml2-config])

AS_IF([test "x$HAVE_XML2CFG" != "x" ],
      [
       xxCFLAGS=`$HAVE_XML2CFG --cflags`
       xxLIBS=`$HAVE_XML2CFG --libs`
       AC_SUBST([LIBXML2_CFLAGS], [$xxCFLAGS])
       AC_SUBST([LIBXML2_LIBS], [$xxLIBS])
       AC_MSG_CHECKING([how to include libxml2 headers])
       AC_MSG_RESULT([$LIBXML2_CFLAGS])
       AC_MSG_CHECKING([how to link with libxml2])
       AC_MSG_RESULT([$LIBXML2_LIBS])
      ])


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
		[],
		[enable_doc=yes])

AS_IF([test "x$enable_doc" != "xno"], 
      [
       # Check for asciidoc (for generating manpages).
       AC_PATH_PROG(A2X, a2x, no)
       AC_PATH_PROG(ASCIIDOC, asciidoc, no)
       AS_IF([test "x$A2X" = "xno"],
          [AC_MSG_WARN([Test for a2x failed. Not generating man pages (but OML build should still complete ok).])])
       AS_IF([test "x$ASCIIDOC" = "xno"],
          [AC_MSG_WARN([Test for asciidoc failed. Not generating man pages (but OML build should still complete ok).])])
      ],[
          AC_MSG_WARN([Not generating man pages (but OML build should still complete ok).])
      ])
AM_CONDITIONAL([ENABLE_DOC], [test "x$enable_doc" != "xno"])
AM_CONDITIONAL([HAVE_A2X], [test "x$A2X" != "xno"])
AM_CONDITIONAL([HAVE_ASCIIDOC], [test "x$ASCIIDOC" != "xno"])

DX_DOT_FEATURE(ON)
DX_PDF_FEATURE(OFF) dnl XXX: Fails not finding existing files...
DX_PS_FEATURE(OFF)
DX_INIT_DOXYGEN([oml2], [], [doxygen])
AC_ARG_ENABLE([doxygen-private],
	      [AS_HELP_STRING([--enable-doxygen-private],
			      [generate Doxygen documentation for private and static elements])],
	      [],
	      [enable_doxygen_private=no])
AS_IF([test x$enable_doxygen_private = xno],
      [
       DX_ENV_APPEND(EXTRACT_PRIVATE, [NO])
       DX_ENV_APPEND(EXTRACT_STATIC, [NO])
      ],
      [
       DX_ENV_APPEND(EXTRACT_PRIVATE, [YES])
       DX_ENV_APPEND(EXTRACT_STATIC, [YES])
      ])
AC_ARG_ENABLE([doxygen-callgraph],
	      [AS_HELP_STRING([--enable-doxygen-callgraph],
			      [generate Doxygen call graphs])],
	      [],
	      [enable_doxygen_callgraph=no])
AS_IF([test x$enable_doxygen_callgraph = xno],
      [DX_ENV_APPEND(ENABLE_CALLGRAPH, [NO])],
      [DX_ENV_APPEND(ENABLE_CALLGRAPH, [YES])])
DX_ENV_APPEND(GENERATE_LATEX, [NO]) dnl generates files not cleaned up, and DX_PDF_FEATURE and DX_PS_FEATURE are currently off
DX_ENV_APPEND(TOPSRCDIR, [\$(top_srcdir)]) dnl let make deal with it


dnl AX_GIT_SEARCH_TREEISH(DESCRIPTION, VARIABLE, COMMA-SEPARATED-LIST, [VALUE_IF_NOT_FOUND])
dnl Copyright (C) 2012 NICTA, Olivier Mehani; under the terms of the GPL2 or later.
dnl Search for the first existing branch of the COMMA-SEPARATED-LIST and return it in VAR, or
dnl set it to VALUE_IF_NOT_FOUND if nono existed e.g.,
dnl   AX_GIT_SEARCH_TREEISH([ArchLinux], [gitarchref],
dnl			    [archlinux/$gittag,
dnl			     origin/archlinux/$gittag
dnl			     ],
dnl			    [origin/archlinux/master])
dnl XXX: This macro is a bit fragile and requires COMMA-SEPARATED-LIST not to
dnl start with a newline.
AC_DEFUN([AX_GIT_SEARCH_TREEISH],[
	 m4_pushdef([DESCRIPTION],[$1])
	 m4_pushdef([VARIABLE],[$2])
	 m4_pushdef([LIST],[$3])
	 m4_pushdef([VALUE_IF_NOT_FOUND],[$4])

	 AC_MSG_CHECKING([for matching DESCRIPTION treeish])
	 m4_foreach([TREEISH], [LIST],[
		    AS_IF([test "x$VARIABLE" = "x" &&
			   $GIT rev-list --max-count=0 TREEISH -- 2>/dev/null],
		    [VARIABLE="TREEISH"])])
	 AS_IF([test "x$VARIABLE" = "x"], [VARIABLE="VALUE_IF_NOT_FOUND"])
	 AC_MSG_RESULT([$VARIABLE])

	 m4_popdef([VALUE_IF_NOT_FOUND])
	 m4_popdef([LIST])
	 m4_popdef([VARIABLE])
	 m4_popdef([DESCRIPTION])
])

AX_WITH_PROG([GIT],[git])
AC_ARG_ENABLE([packaging],
	      [AS_HELP_STRING([--enable-packaging],
			      [enable targets to create distribution-specific packages (Git clone needed)])],
	      [AS_IF([test "x$enable_packaging" != "xno"],
		     [
		     AC_MSG_CHECKING([whether this is a Git repo])
		     AS_IF([$GIT status >/dev/null 2>&1],
			   [
			    AC_MSG_RESULT([yes])
			    AC_MSG_CHECKING([current Git branch])
			    gitbranch=`${GIT} describe --contains --all HEAD`
			    AC_MSG_RESULT([$gitbranch])
			    AC_MSG_CHECKING([current Git tag])
			    gittag=`${GIT} describe --exactmatch 2>/dev/null`
			    AS_IF([ test "x" = "x$gittag"],
			          [
			           gitistag=no
			           AC_MSG_RESULT([none])
			           AC_MSG_CHECKING([closest Git tag])
			           gittag=`${GIT} describe --abbrev=0 2>/dev/null`
			           AC_MSG_RESULT([$gittag])
				   AX_WITH_PROG([MAKEPKG],[makepkg])
				   AS_IF([test "x$MAKEPKG" != "x"],[
					  AX_GIT_SEARCH_TREEISH([ArchLinux],
								[gitarchref],
								[archlinux/$gittag,
								 staging/archlinux/$gittag,
								 origin/archlinux/$gittag,
								 archlinux/$gitbranch,
								 staging/archlinux/$gitbranch,
								 origin/archlinux/$gitbranch,
								 archlinux/master,
								 staging/archlinux/master],
								[origin/archlinux/master])
					  ])
				   AX_WITH_PROG([DPKG_BUILDPACKAGE],[dpkg-buildpackage])
				   AX_WITH_PROG([DCH],[dch])
				   AS_IF([test "x$DPKG_BUILDPACKAGE" != "x" -a "x$DCH" != "x"],[
					  AX_GIT_SEARCH_TREEISH([Debian],
								[gitdebref],
								[debian/$gittag,
								 staging/debian/$gittag,
								 origin/debian/$gittag,
								 debian/$gitbranch,
								 staging/debian/$gitbranch,
								 origin/debian/$gitbranch,
								 debian/master,
								 staging/debian/master],
								[origin/debian/master])
					  ])
				   AX_WITH_PROG([RPMBUILD],[rpmbuild])
				   AS_IF([test "x$RPMBUILD" != "x"], [
					  AX_GIT_SEARCH_TREEISH([RPM],
								[gitrpmref],
								[rpm/$gittag,
								 staging/rpm/$gittag,
								 origin/rpm/$gittag,
								 rpm/$gitbranch,
								 staging/rpm/$gitbranch,
								 origin/rpm/$gitbranch,
								 rpm/master,
								 staging/rpm/master],
								[origin/rpm/master])
					  ])
				   ])
				  ],
			          [
			           AC_MSG_RESULT([$gittag])
			           gitistag=yes
			          ])
			    ],
			    [
			     AC_MSG_RESULT([no])
			     AC_MSG_WARN([--enable-packaging needs a Git repo])
			     enable_packaging=no
			    ])
	      ],
	      [enable_packaging=no])
AC_SUBST([GITBRANCH],[$gitbranch])
AC_SUBST([GITTAG],[$gittag])
AC_SUBST([GITHEAD],[$githead])
AC_SUBST([GITARCHREF],[$gitarchref])
AC_SUBST([GITDEBREF],[$gitdebref])
AC_SUBST([GITRPMREF],[$gitrpmref])
AM_CONDITIONAL([ENABLE_PACKAGING], [test "x$enable_packaging" != "xno"])
AM_CONDITIONAL([GITISTAG], [test "x$gitistag" != "xno"])
AM_CONDITIONAL([BUILD_ARCH], [test "x$MAKEPKG" != "x"])
AM_CONDITIONAL([BUILD_DEBIAN], [test "x$DPKG_BUILDPACKAGE" != "x" -a "x$DCH" != "x"])
AM_CONDITIONAL([BUILD_RPM], [test "x$RPMBUILD" != "x"])

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

AS_IF([test "x$missing_check" != "x"],
   [AC_MSG_WARN([could not find the `check' package, required for running unit tests. This will not stop OML from building, but it will cause `make check' to do nothing. Install the `check' package (http://check.sourceforge.net) if you wish to run the unit tests.])])

# Let the user know we're missing required libraries, and don't generate the build system.
AS_IF([test "x$missing_libs" != "x"],
   [
    for lib in $missing_libs; do
      AC_MSG_WARN([could not find required library $lib])
   done
   AC_MSG_ERROR([missing libraries, could not continue. Please install all of the following libraries and then re-run configure (you might need the development header packages (-dev) as well if you are using your system's package manager to install them): $missing_libs])
  ])

AC_OUTPUT
