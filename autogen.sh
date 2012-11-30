#!/bin/sh
rm -rf autom4te.cache/
# Let the user know what we do, tersely
echo "gnulib-tool: updating portability libary files" >&2
gnulib-tool --update --quiet --quiet >/dev/null
chmod a+x build-aux/git-version-gen
autoreconf -i $*
