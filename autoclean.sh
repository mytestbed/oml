#!/bin/sh

RM=`which rm`

set -x

test -f Makefile && make distclean

test -d autom4te.cache && ${RM} -fr autom4te.cache

find . -name Makefile.in -exec ${RM} -f {} +
${RM} -f aclocal.m4 config.h.in config.guess config.sub configure depcomp install-sh ltmain.sh missing m4/*.m4

