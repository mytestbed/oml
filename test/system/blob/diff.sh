#!/bin/sh

dir=$1
prefix=
fail=


if [ "x$dir" = "x" ]; then
	prefix=""
else
	prefix=${dir}/
fi

echo -n "Checking that server stored blobs match client-generated blobs:"
for f in ${prefix}g*.hex; do
	g=$(basename $f .hex)
	s="s${g#g}"
	if diff -qw ${prefix}$g.hex ${prefix}$s.hex; then
		echo -n " $g==$s"
	else
		fail=yes
		printf "FAIL:$g!=$sL\n"
	fi
done
echo "."

test "x$fail" != "xyes"
