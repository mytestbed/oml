#!/bin/bash
# Self-instrumentation tests.
#
# Copyright 2013-2015 National ICT Australia (NICTA), Steve Glass, Olivier Mehani
#
# This software may be used and distributed solely under the terms of
# the MIT license (License).  You should find a copy of the License in
# COPYING or at http://opensource.org/licenses/MIT. By downloading or
# using this software you accept the terms and the liability disclaimer
# in the License.
#
# Can be run manually as
#  top_srcdir=../.. srcdir=. top_builddir=../.. builddir=. ./self-inst.sh
#
N=${0/#.\//$PWD\/}		# Get script name, replacing ./ with a full path
srcdir=${srcdir/#./$PWD\/.}	# Ditto for directories
top_builddir=${top_builddir/#./$PWD\/.}
builddir=${builddir/#./$PWD\/.}

BN=`basename $0`
DOMAIN=${BN%%.sh}
SQLDB="${DOMAIN}.sq3"

LOG=$PWD/${DOMAIN}.log
source ${srcdir}/tap_helper.sh

tap_message "testing self-instrumentation"

test_plan

tap_test "make temporary directory" yes mktemp -d ${DOMAIN}.XXXXXX
DIR=`tail -n 1 $LOG` # XXX: $LOG cannot be /dev/stdout here
cd $DIR
tap_message "working in $DIR; it won't be cleaned up in case of bail out"

port=$((RANDOM + 32766))

# Start an OML server
${top_builddir}/server/oml2-server -l $port -D . --oml-collect localhost:$port --oml-domain "$DOMAIN" --oml-id oml2-server > ${BN}_server.log 2>&1 &
tap_test "start ${top_builddir}/server/oml2-server" yes test x$? == x0
OMLPID=$!
tap_message "started OML server with PID $OMLPID (might need manual killing if the suit bails out); waiting a bit..."
sleep 5

tap_test "start ${srcdir}/self-inst.py" yes ${srcdir}/self-inst.py --oml-collect localhost:$port 2>&1
tap_message "waiting a bit more..."
sleep 20
tap_test "terminate OML server" no kill -TERM $OMLPID
tap_test "wait for OML server to die" no wait $OMLPID

tap_test "confirm existence of $SQLDB" yes test -e "${SQLDB}"

tap_test "find recorded clients" no test `sqlite3 "${SQLDB}" "SELECT COUNT(*) FROM server_clients;"` -gt 0
# XXX: at the moment, only the C library has self instrumentation; fortunately, the server reports this to itself.
tap_test "find self-intrumentation reports from client" no test `sqlite3 "${SQLDB}" "SELECT COUNT(*) FROM _client_instrumentation;"` -gt 0

cd - >/dev/null
tap_message "cleaning $DIR"
rm -rf $DIR

tap_summary
