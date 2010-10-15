#!/bin/sh

fail=

for f in g*.hex; do
	g=$(basename -s .hex $f)
	s="s${g#g}"
	if diff -qw $g.hex $s.hex; then
		echo "$g <==> $s OK"
	else
		fail=yes
		echo "$g <!=> $s FAIL"
	fi
done

test "x$fail" = "xyes"