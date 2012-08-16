#!/bin/sh -x

test -f Makefile && make distclean

test -d autom4te.cache && rm -fr autom4te.cache

find . -name Makefile.in -exec rm -f {} +
rm -rf aclocal.m4 build-aux/ config.h.in configure m4/00gnulib.m4 m4/gnulib-com* m4/gnulib-tool.m4 m4/lt*.m4
