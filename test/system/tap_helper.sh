# Rudimentary TAP output helper
#
# Copyright 2013 National ICT Australia (NICTA), Olivier Mehani
#
# This software may be used and distributed solely under the terms of
# the MIT license (License).  You should find a copy of the License in
# COPYING or at http://opensource.org/licenses/MIT. By downloading or
# using this software you accept the terms and the liability disclaimer
# in the License.
#
# Source this file into any shell script needing to run test and output TAP.
#
# Example use:
#
#   LOG=blah # defaults to /dev/stderr
#   source ${srcdir}/tap_helper.sh # just in case this is used with automake... just in case
#   tap_message "let's get testing"
#   tap_test "bail out if this fails" yes /bin/true
#   tap_test "keep going if that fails" no /bin/false
#   tap_skip "mention that a test has been skipped" no /bin/false
#   tap_test "test an expected failure XFAIL <- this last string does the trick" no /bin/false
#   tap_message "we're done"
#

test_plan()
{
	echo 1..`grep ^[^#]*tap_test $0 | grep -v "()" | wc -l`
}

tap_message()
{
	echo "# $*"
}

tap_test()
{
	DESC=$1
	shift
	FATAL=$1 # no if not fatal
	shift
	CMD=$*

	tested=$((tested + 1))
	echo "$ $CMD" >> $LOG
	$CMD >> $LOG 2>&1
	if [ x$? = x0 ]; then
		echo "ok $tested - $DESC"

	elif [ x$FATAL = xno ]; then
		echo "not ok $tested - cannot $DESC"
		failed=$((failed + 1))

	else
		echo "Bail out! - cannot $DESC; check $LOG"
		exit 1
	fi
}

tap_skip()
{
	DESC=$1
	tested=$((tested + 1))
	echo "not ok $tested - SKIP: $DESC"
	skipped=$((skipped + 1))
}

tap_summary()
{
	tap_message "$failed/$tested tests failed; $skipped skipped"
}

tap_absolutise()
{
  # Equivalent of ${1/#.\//$PWD\/} in bash: get script name, replacing ./ with a full path
  echo $1 | sed "s?^\.?$PWD/.?"
}

if [ -z "$LOG" ]; then
	tap_message "\$LOG not set; using /dev/stderr"
	LOG=/dev/stderr
else
	LOG=`tap_absolutise $LOG`
fi

tested=0
failed=0
skipped=0
