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
#  top_srcdir=../.. srcdir=. top_builddir=../.. builddir=. TIMEOUT=`which timeout` ./reconnect.sh [--text] [--gzip]
#
N=${0/#.\//$PWD\/}		# Get script name, replacing ./ with a full path
srcdir=${srcdir/#./$PWD\/.}	# Ditto for directories
top_builddir=${top_builddir/#./$PWD\/.}
builddir=${builddir/#./$PWD\/.}

BN=`basename $0`
DOMAIN=${BN%%.sh}
MODE=binary
OUTFILTER=cat
DATACHECK=tap_test
while [ $# -ge 0 ] ; do
	case $1 in
		"--text")
			OPTARGS=--oml-text
			DOMAIN=${DOMAIN}-text
			MODE=${MODE/binary/text} # Avoid losing a potential suffix
			;;
		"--gzip")
			OPTSCHEME=gzip+
			DOMAIN=${DOMAIN}-gzip
			MODE=${MODE}-gzip
			OUTFILTER="${builddir}/inflate"
			# As Gzip data is bundled together, we cannot guarantee that no data will be lost
			DATACHECK=tap_skip
			;;
	esac
	shift || break
done

LOG=$PWD/${DOMAIN}.log
source ${srcdir}/tap_helper.sh

OMSPDUMP="${DOMAIN}.omsp"
NSAMPLES=10
GENINTERVAL=1000 #ms
TO=$((NSAMPLES*GENINTERVAL/900)) # That's a banker's second
if [ ! -z "${TIMEOUT}" ]; then
	TIMEOUT="${TIMEOUT} -k ${TO}s $((TO*3))s"
else
	tap_message "timeout(1) utility not found; this test might hang indefinitely"
fi

# On Debian-ishes, we need netcat-openbsd;
# it can also be set to be the default with
# sudo update-alternatives --set nc /bin/nc.openbsd
NC=`which nc.openbsd 2>/dev/null|| echo nc`

tap_message "testing reconnection for ${MODE} mode"

test_plan

tap_test "make temporary directory" yes mktemp -d ${DOMAIN}.XXXXXX
DIR=`tail -n 1 $LOG` # XXX: $LOG cannot be /dev/stdout here
cd $DIR
tap_message "working in $DIR; it won't be cleaned up in case of bail out"

port=$((RANDOM + 32766))

# Start a fake OML server
# NOTE: -d prevents OpenBSD nc from reading from STDIN,
# which confuses it into (sometimes) closing the reverse connection to the
# client when put in the background, which in turns leads the client
# to disconnect
${NC} -ld $port > ${OMSPDUMP}.out &
tap_test "start netcat" yes test x$? == x0
NCPID=$!
tap_message "started $NC with PID $NCPID (might need manual killing if the suit bails out)"

${TIMEOUT} ${builddir}/blobgen -f 10 -n ${NSAMPLES} -i ${GENINTERVAL} ${OPTARGS} --oml-id generator --oml-domain ${DOMAIN} --oml-collect ${OPTSCHEME}tcp://localhost:$port > ${DOMAIN}-client.log 2>&1 &
tap_test "start blobgen" yes test x$? == x0
GENPID=$!
tap_message "started blobgen with PID $GENPID (might need manual killing if the suit bails out)"
tap_message "waiting a bit..."
sleep 7

tap_test "kill netcat ($NCPID)" no kill $NCPID
tap_message "waiting some more..."
sleep 2

${NC} -ld $port >> ${OMSPDUMP}.out &
tap_test "start new netcat" yes test x$? == x0
NCPID=$!
tap_message "started new $NC with PID $NCPID (might need manual killing if the suit bails out)"

tap_message "waiting for $GENPID to finish..."
while kill -0 $GENPID; do
	sleep 1
done

tap_message "killing netcat ($NCPID), in case it's still around..."
kill $NCPID
tap_message "waiting some more..."
sleep 2

tap_test "post-process output to plain OMSP" yes $(${OUTFILTER} ${OMSPDUMP}.out > ${OMSPDUMP})
tap_test "confirm that headers were sent twice" yes test `grep -a start-time ${OMSPDUMP} | wc -l` -eq 2
eval "${DATACHECK} \"confirm that all data was received\" yes test `strings ${OMSPDUMP} | grep sample- | wc -l` -eq ${NSAMPLES}" # The generator has two string outputs

#tap_message "full OMSP follows"
#cat ${OMSPDUMP}
#tap_message "full OMSP ends"

cd - >/dev/null
tap_message "cleaning $DIR"
rm -rf $DIR

tap_summary
