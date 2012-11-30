#!/bin/sh -x

test -f Makefile && make maintainer-clean

find . -name Makefile.in -exec rm -f {} +
find build-aux/ -type f -not -name gen-authors.sh -exec rm {} +
rm -rf aclocal.m4 config.h.in configure gnulib/ m4/00gnulib.m4 m4/gnulib-com* m4/gnulib-tool.m4 m4/lt*.m4 m4/libtool.m4
