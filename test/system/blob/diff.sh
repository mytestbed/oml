#!/bin/sh

fail=

printf "Checking that server stored blobs match client-generated blobs\n"

for f in g*.hex; do
	g=$(basename -s .hex $f)
	s="s${g#g}"
	if diff -qw $g.hex $s.hex; then
		printf "\r$g <==> $s OK      "
	else
		fail=yes
		printf "\n$g <!=> $s FAIL\n"
	fi
done

printf "\r...done         \n"

test "x$fail" != "xyes"
