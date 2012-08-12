#!/bin/sh
gnulib-tool --update --quiet --quiet
chmod a+x build-aux/git-version-gen
autoreconf -i $*
