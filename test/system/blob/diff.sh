#!/bin/sh

dir=$1
prefix=
fail=


if [ "x$dir" = "x" ]; then
	prefix=""
else
	prefix=${dir}/
fi

printf "Checking that server stored blobs match client-generated blobs\n"

for f in ${prefix}g*.hex; do
	g=$(basename $f .hex)
	s="s${g#g}"
	if diff -qw ${prefix}$g.hex ${prefix}$s.hex; then
		printf "\r$g <==> $s OK      "
	else
		fail=yes
		printf "\n$g <!=> $s FAIL\n"
	fi
done

printf "\r...done         \n"

test "x$fail" != "xyes"
