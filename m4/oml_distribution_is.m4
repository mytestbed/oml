dnl OML_DISTRIBUTION_IS(DISTRIBUTION_NAME, FILE, VARIABLE, [ACTION_IF_FOUND], [ACTION_IF_NOT_FOUND])
dnl Copyright (C) 2012-2013 NICTA, Olivier Mehani; under the terms of the GPL2 or later.
dnl Search for a file characteristic of a specific distribution, and set the
dnl VARIABLE to the content of that file if found, before executing
dnl ACTION_IF_FOUND, or `no' and ACTION_IF_NOT_FOUND otherwise.
dnl   OML_DISTRIBUTION_IS([ArchLinux], [/etc/arch-release], [isarch]
dnl			    [AC_MSG_NOTICE([Build host is ArchLinux])],
dnl			    [AC_MSG_WARN([Build host is NOT ArchLinux])])
AC_COPYRIGHT([Copyright (C) 2012-2013 NICTA, Olivier Mehani.])
AC_DEFUN([OML_DISTRIBUTION_IS],[
	 m4_pushdef([DESCRIPTION],[$1])
	 m4_pushdef([FILE],[$2])
	 m4_pushdef([VARIABLE],[$3])
	 m4_pushdef([ACTION_IF_FOUND],[$4])
	 m4_pushdef([ACTION_IF_NOT_FOUND],[$5])

	 AC_MSG_CHECKING([if build host OS is DESCRIPTION (i.e., FILE exists)]) 
	 AS_IF([test -e FILE], [
		 VARIABLE=`cat FILE`
		 test -z "$VARIABLE" && VARIABLE="yes"
		 AC_MSG_RESULT([$VARIABLE])
		 ACTION_IF_FOUND
		 ], [
		 VARIABLE=""
		 AC_MSG_RESULT([no])
		 ACTION_IF_NOT_FOUND
		 ])

	 m4_popdef([ACTION_IF_NOT_FOUND])
	 m4_popdef([ACTION_IF_FOUND])
	 m4_popdef([VARIABLE])
	 m4_popdef([FILE])
	 m4_popdef([DESCRIPTION])
])
