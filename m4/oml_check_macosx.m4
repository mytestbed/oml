dnl OML_CHECK_MACOSX
dnl Copyright (C) 2012-2013 NICTA, Olivier Mehani; under the terms of the GPL2 or later.
dnl This macro checks whether the system is OS X, and searches for Homebrew,
dnl MacPorts and Fink paths (in this order).
AC_COPYRIGHT([Copyright (C) 2012-2013 NICTA, Olivier Mehani.])
AC_DEFUN([OML_CHECK_MACOSX], [ 
	  AC_ARG_WITH([macosx], 
		      [AS_HELP_STRING([--with-macosx],
				      [search in default MacPorts, Homebrew and Fink paths for Mac OS X @<:@default=auto@:>@])],
		      [],
		      [with_macosx=check])

	  AC_MSG_CHECKING([if host is Mac OS X])
	  AS_IF([test x$with_macosx != xno -a $host_vendor = apple],
		[AS_CASE([$host_os],
			 [darwin*],[
			  AC_MSG_RESULT([yes])
			  AC_MSG_NOTICE([adding existing search paths (use --with-macosx=no and manually set the variables to override)...])

			  for thepath in \
				  /usr/local/bin \
				  /opt/local/bin \
				  /sw/bin
			  do
				  AC_MSG_CHECKING([whether $thepath exists (\$PATH)])
				  AS_IF([test -d $thepath],[
					 AC_MSG_RESULT([yes])
					 PATH=$thepath:$PATH
					 ],
					 [AC_MSG_RESULT([no])])
			  done

			  for thepath in \
				  /usr/local/include \
				  /opt/local/include \
				  /sw/include
			  do
				  AC_MSG_CHECKING([whether $thepath exists (\$CFLAGS)])
				  AS_IF([test -d $thepath],[
					 AC_MSG_RESULT([yes])
					 CFLAGS="-I$thepath $CFLAGS"
					 ],
					 [AC_MSG_RESULT([no])])
			  done

			  for thepath in \
				  /usr/local/lib \
				  /opt/local/lib \
				  /sw/lib
			  do
				  AC_MSG_CHECKING([whether $thepath exists (\$LDFLAGS)])
				  AS_IF([test -d $thepath],[
					 AC_MSG_RESULT([yes])
					 LDFLAGS="-L$thepath $LDFLAGS"
					 ],
					 [AC_MSG_RESULT([no])])
			  done 
			  ],
			  [AC_MSG_RESULT([no])])
		],
		[AC_MSG_RESULT([no])])
	  ])
