dnl OML_GIT_SEARCH_TREEISH(DESCRIPTION, VARIABLE, COMMA-SEPARATED-LIST,
dnl [VALUE_IF_NOT_FOUND]) Copyright (C) 2012-2014 NICTA, Olivier Mehani; under
dnl the terms of the GPL2 or later.  Search for the first existing branch (or
dnl tag if SEARCH_TAGS is yes) of the COMMA-SEPARATED-LIST and return it in VAR,
dnl or set it to VALUE_IF_NOT_FOUND if nono existed e.g.,
dnl  OML_GIT_SEARCH_TREEISH([ArchLinux], [gitarchref], [$gitistag],
dnl			    [archlinux/$gittag,
dnl			     origin/archlinux/$gittag
dnl			     ],
dnl			    [origin/archlinux/master])
dnl XXX: This macro is a bit fragile and requires COMMA-SEPARATED-LIST not to
dnl start with a newline.
AC_COPYRIGHT([Copyright (C) 2012-2014 NICTA, Olivier Mehani.])
AC_DEFUN([OML_GIT_SEARCH_TREEISH],[
	 m4_pushdef([DESCRIPTION],[$1])
	 m4_pushdef([VARIABLE],[$2])
	 m4_pushdef([SEARCH_TAGS],[$3])
	 m4_pushdef([LIST],[$4])
	 m4_pushdef([VALUE_IF_NOT_FOUND],[$5])

	 AC_MSG_CHECKING([for matching DESCRIPTION treeish])
	 m4_foreach([TREEISH], [LIST], [
		 treeish=`echo TREEISH | sed 's@\([^^]\)\(remotes\|origin\)/@\1@g'`
		 AS_IF([test "x$VARIABLE" = "x"], [
			 AS_IF([test "SEARCH_TAGS"="yes"],[
				 TAG=`$GIT tag --list ${treeish} ${treeish}-* 2>/dev/null | sort | tail -n 1`
				 ])
			 BRANCH=`$GIT branch --list ${treeish} 2>/dev/null | sort | tail -n 1`
			 AS_IF([test "x$TAG" != "x"],
				 [VARIABLE=$TAG],
				 [test "x$BRANCH" != "x"],
				 [VARIABLE=$BRANCH],
				 [VARIABLE=""]
			      )
			 ])
		 ])
	 AS_IF([test "x$VARIABLE" = "x"], [VARIABLE="VALUE_IF_NOT_FOUND"])
	 REV=`git rev-list -n 1 ${VARIABLE}`
	 AC_MSG_RESULT([$VARIABLE ($REV)])

	 m4_popdef([VALUE_IF_NOT_FOUND])
	 m4_popdef([LIST])
	 m4_popdef([SEARCH_TAGS])
	 m4_popdef([VARIABLE])
	 m4_popdef([DESCRIPTION])
])


