#!/bin/bash
# Reconnection tests.
#
# Copyright 2014 National ICT Australia (NICTA), Olivier Mehani
#
# This software may be used and distributed solely under the terms of
# the MIT license (License).  You should find a copy of the License in
# COPYING or at http://opensource.org/licenses/MIT. By downloading or
# using this software you accept the terms and the liability disclaimer
# in the License.
#
# Can be run manually as
#  top_srcdir=../.. srcdir=. top_builddir=../.. builddir=. ./reconnect.sh [--text]
#
N=${0/#.\//$PWD\/}		# Get script name, replacing ./ with a full path
srcdir=${srcdir/#./$PWD\/.}	# Ditto for directories
top_builddir=${top_builddir/#./$PWD\/.}
builddir=${builddir/#./$PWD\/.}

BN=`basename $0`
DOMAIN=${BN%%.sh}
if [ "$1" = "--text" ]; then
	OPTARGS=--oml-text
	DOMAIN=${DOMAIN}-text
	MODE=text
else
	MODE=binary
fi

OMSPDUMP="${DOMAIN}.omsp"
n=15

LOG=$PWD/${DOMAIN}.log
source ${srcdir}/tap_helper.sh

# On Debian-ishes, we need netcat-openbsd;
# it can also be set to be the default with
# sudo update-alternatives --set nc /bin/nc.openbsd
NC=`which nc.openbsd 2>/dev/null|| echo nc`

tap_message "testing reconnection for ${MODE} mode"

test_plan

tap_test "make temporary directory" yes mktemp -d ${DOMAIN}-test.XXXXXX
DIR=`tail -n 1 $LOG` # XXX: $LOG cannot be /dev/stdout here
cd $DIR
tap_message "working in $DIR; it won't be cleaned up in case of bail out"

port=$((RANDOM + 32766))

# Start a fake OML server
# NOTE: -d prevents OpenBSD nc from reading from STDIN,
# which confuses it into (sometimes) closing the reverse connection to the
# client when put in the background, which in turns leads the client
# to disconnect
${NC} -ld $port > ${OMSPDUMP} &
tap_test "start netcat" yes test x$? == x0
NCPID=$!
tap_message "started $NC with PID $NCPID (might need manual killing if the suit bails out)"

${builddir}/blobgen -f 10 -n $n -i 1000 ${OPTARGS} --oml-id generator --oml-domain ${DOMAIN} --oml-collect localhost:$port > ${DOMAIN}-client.log 2>&1 &
tap_test "start blobgen" yes test x$? == x0
GENPID=$!
tap_message "started blobgen with PID $GENPID (might need manual killing if the suit bails out)"
tap_message "waiting a bit..."
sleep 7

tap_test "kill netcat ($NCPID)" no kill $NCPID
tap_message "waiting some more..."
sleep 2

${NC} -ld $port >> ${OMSPDUMP} &
tap_test "start new netcat" yes test x$? == x0
NCPID=$!
tap_message "started new $NC with PID $NCPID (might need manual killing if the suit bails out)"

tap_message "waiting some more..."
sleep 10

tap_message "killing netcat ($NCPID), in case it's still around..."
kill $NCPID
tap_message "waiting some more..."
sleep 2

tap_test "confirm that headers were sent twice" yes test `grep -a start-time ${OMSPDUMP} | wc -l` -eq 2
tap_test "confirm that all data was received" yes test `strings ${OMSPDUMP} | grep sample- | wc -l` -eq $n # The generator has two string outputs

#tap_message "full OMSP follows"
#cat ${OMSPDUMP}
#tap_message "full OMSP ends"

cd - >/dev/null
tap_message "cleaning $DIR"
rm -rf $DIR

tap_summary
