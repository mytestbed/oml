#!/bin/sh
rm -rf autom4te.cache/
gnulib-tool --update --quiet --quiet
chmod a+x build-aux/git-version-gen
autoreconf -i $*
