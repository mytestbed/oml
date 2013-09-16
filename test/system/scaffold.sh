#!/bin/sh
#
# This script tests that oml2-scaffold can be used as per the tutorial [0].
#
# [0] http://oml.mytestbed.net/projects/oml/wiki/Quick_Start_Tutorial
#
# Can be run manually as
#  top_srcdir=../.. top_builddir=../.. TIMEOUT=`which timeout` [SCAFFOLD=/full/path/to/scaffold] ./run.sh

APPNAME=generator
BASEDIR=$PWD
LOG=$PWD/scaffold.log

if [ -z "$SCAFFOLD" ]; then
	SCAFFOLD="ruby $top_builddir/../ruby/oml2-scaffold"
fi

echo -n > $LOG

#
# Rudimentary TAP output helpers
#
function test_plan()
{
	echo 1..`grep ^[^#]*tap_test $0 | grep -v function | wc -l`
}

function tap_test()
{
	DESC=$1
	shift
	FATAL=$1 # no if not fatal
	shift
	CMD=$*

	echo "$ $CMD" >> $LOG
	$CMD >> $LOG 2>&1
	if [ x$? = x0 ]; then
		echo "ok $((++i)) - $DESC"

	elif [ x$FATAL = xno ]; then
		echo "not ok $((++i)) - cannot $DESC"
		failed=$((failed + 1))

	else
		echo "Bail out! - cannot $DESC; check $LOG"
		exit 1
	fi
}

function tap_skip()
{
	DESC=$1
	echo "not ok $((++i)) - SKIP: $DESC"
	skipped=$((skipped + 1))
}

function tap_summary()
{
	echo "# $failed/$i tests failed; $skipped skipped"
}

tested=0
failed=0
skipped=0


echo "# testing scaffolding script; running with \`$SCAFFOLD'"

test_plan

tap_test "making temporary directory" yes mktemp -d scaffold-test.XXXXXX
DIR=`tail -n 1 $LOG`
cd $DIR
echo "# working in $DIR; it won't be cleaned up in case of bail out"

#tap_test "generate AppDefinition skeleton" yes $SCAFFOLD --app $APPNAME
tap_test "generate AppDefinition skeleton XFAIL due to #1444" no $SCAFFOLD --app $APPNAME

#tap_test "generate main file" yes $SCAFFOLD --main ${APPNAME}.rb
tap_test "generate main file XFAIL due to #1444" no $SCAFFOLD --main ${APPNAME}.rb

#tap_test "generate Makefile" yes $SCAFFOLD --make ${APPNAME}.rb
tap_test "generate Makefile XFAIL due to #1444" no $SCAFFOLD --make ${APPNAME}.rb

#tap_test "generate OML header" no $SCAFFOLD --oml ${APPNAME}.rb
#tap_test "generate popt(3) header" no $SCAFFOLD --opts ${APPNAME}.rb
tap_test "generate OML header XFAIL due to #1444" no $SCAFFOLD --oml ${APPNAME}.rb
tap_test "generate popt(3) header XFAIL due to #1444" no $SCAFFOLD --opts ${APPNAME}.rb

export SCAFFOLD
export CC="$top_builddir/../libtool compile gcc"
export CCLD="$top_builddir/../libtool link --tag=CC gcc"
export CFLAGS="-I$top_srcdir/../lib/client"
export LDFLAGS="-L$top_builddir/../lib/client/.libs -L$top_builddir/../lib/ocomm/.libs"
#tap_test "build generated application" yes make -e
tap_test "build generated application XFAIL due to #1444" no make -e

if [ -z "$TIMEOUT" ]; then
	tap_skip "TIMEOUT not specified"

else
	#tap_test "run generated application" yes $TIMEOUT --preserve-status -s INT 5 \
	tap_test "run generated application XFAIL due to #1444" no $TIMEOUT --preserve-status -s INT 5 \
		./$APPNAME --oml-id genid --oml-domain gendomain --oml-collect file:-
fi

cd - >/dev/null
echo "# cleaning $DIR"
rm -rf $DIR

tap_summary
