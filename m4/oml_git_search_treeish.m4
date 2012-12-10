dnl OML_GIT_SEARCH_TREEISH(DESCRIPTION, VARIABLE, COMMA-SEPARATED-LIST, [VALUE_IF_NOT_FOUND])
dnl Copyright (C) 2012 NICTA, Olivier Mehani; under the terms of the GPL2 or later.
dnl Search for the first existing branch of the COMMA-SEPARATED-LIST and return it in VAR, or
dnl set it to VALUE_IF_NOT_FOUND if nono existed e.g.,
dnl  OML_GIT_SEARCH_TREEISH([ArchLinux], [gitarchref],
dnl			    [archlinux/$gittag,
dnl			     origin/archlinux/$gittag
dnl			     ],
dnl			    [origin/archlinux/master])
dnl XXX: This macro is a bit fragile and requires COMMA-SEPARATED-LIST not to
dnl start with a newline.
AC_COPYRIGHT([Copyright (C) 2012 NICTA, Olivier Mehani.])
AC_DEFUN([OML_GIT_SEARCH_TREEISH],[
	 m4_pushdef([DESCRIPTION],[$1])
	 m4_pushdef([VARIABLE],[$2])
	 m4_pushdef([LIST],[$3])
	 m4_pushdef([VALUE_IF_NOT_FOUND],[$4])

	 AC_MSG_CHECKING([for matching DESCRIPTION treeish])
	 m4_foreach([TREEISH], [LIST], [
		 treeish=`echo TREEISH | sed 's@\([^^]\)\(remotes\|origin\)/@\1@g'`
		 AS_IF([test "x$VARIABLE" = "x"], [
			 TAG=`$GIT tag --list ${treeish} ${treeish}-* 2>/dev/null | sort | tail -n 1`
			 BRANCH=`$GIT branch --list ${treeish} ${treeish}-* 2>/dev/null | sort | tail -n 1`
			 AS_IF([test "x$TAG" != "x"],
				 [VARIABLE=$TAG],
				 [test "x$BRANCH" != "x"],
				 [VARIABLE=$BRANCH],
				 [VARIABLE=""]
			      )])])
	 AS_IF([test "x$VARIABLE" = "x"], [VARIABLE="VALUE_IF_NOT_FOUND"])
	 AC_MSG_RESULT([$VARIABLE])

	 m4_popdef([VALUE_IF_NOT_FOUND])
	 m4_popdef([LIST])
	 m4_popdef([VARIABLE])
	 m4_popdef([DESCRIPTION])
])


