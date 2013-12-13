#!/bin/sh
#
# This script tests that oml2-scaffold can be used as per the tutorial [0].
#
# [0] http://oml.mytestbed.net/projects/oml/wiki/Quick_Start_Tutorial
#
# Can be run manually as
#  top_srcdir=../.. srcdir=. top_builddir=../.. builddir=. TIMEOUT=`which timeout` [SCAFFOLD=/full/path/to/scaffold] [da]sh ./scaffold.sh

absolutise()
{
  # Equivalent of ${1/#.\//$PWD\/} in bash: get script name, replacing ./ with a full path
  echo $1 | sed "s?^\.?$PWD/.?"
}

N=`absolutise $0`
top_srcdir=`absolutise $top_srcdir`
srcdir=`absolutise $srcdir`
top_builddir=`absolutise $top_builddir`
builddir=`absolutise $builddir`

BN=`basename $0`
APPNAME=generator

LOG=$PWD/${BN%%sh}log
. ${srcdir}/tap_helper.sh

if [ -z "$SCAFFOLD" ]; then
	SCAFFOLD="ruby $top_builddir/ruby/oml2-scaffold"
fi

echo -n > $LOG


tap_message "testing scaffolding script; running with \`$SCAFFOLD'"

test_plan

tap_test "make temporary directory" yes mktemp -d scaffold-test.XXXXXX
DIR=`tail -n 1 $LOG` # XXX: $LOG cannot be /dev/stdout here
cd $DIR
tap_message "working in $DIR; it won't be cleaned up in case of bail out"

tap_test "generate AppDefinition skeleton" yes $SCAFFOLD --app $APPNAME

tap_test "generate main file" yes $SCAFFOLD --main ${APPNAME}.rb

tap_test "generate Makefile" yes $SCAFFOLD --make ${APPNAME}.rb

tap_test "generate OML header" no $SCAFFOLD --oml ${APPNAME}.rb
tap_test "generate popt(3) header" no $SCAFFOLD --opts ${APPNAME}.rb

export SCAFFOLD
export CC="$top_builddir/libtool compile gcc"
export CCLD="$top_builddir/libtool link --tag=CC gcc"
export CFLAGS="-I$top_srcdir/lib/client -I$top_srcdir/lib/ocomm"
export LDFLAGS="-L$top_builddir/lib/client/.libs -L$top_builddir/lib/ocomm/.libs"
tap_test "build generated application" yes make -e

testrun()
{
	($TIMEOUT -k 10s -s INT 5 \
		./$APPNAME --oml-id genid --oml-domain gendomain --oml-collect file:- \
		|| test $? = 124) \
		&& return 0
}

if [ -z "$TIMEOUT" ]; then
	tap_skip "TIMEOUT not specified"

else
	export LD_LIBRARY_PATH="$top_builddir/lib/client/.libs:$top_builddir/lib/ocomm/.libs"
	tap_test "run generated application" yes testrun
fi

cd - >/dev/null
tap_message "cleaning $DIR"
rm -rf $DIR

tap_summary
