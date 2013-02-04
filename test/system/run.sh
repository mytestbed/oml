#!/bin/bash
#
# This script is a meta test of the whole OML reporting chain.
#
# It spawns a server with the selected backend---given as the first argument
# (sq3 or pg), defaulting to SQLite3---and a blob-generating client.
#
# The client also writes the generated blobs on disk, and can generate longer
# ones with the --long flag (as the second argument to this script).
# 
# Once done, the script extracts the data from the storage backend, and
# compares them to those dumped by the client.
#
# It gives the number of mismatches as its return status. It purposefully
# doesn't clean up after itself (see Makefile's CLEANLOCAL target) to allow for
# forensic inspection in case of failures.
#
# Can be run manually as
#  top_builddir=../.. POSTGRES=`which postgres` TIMEOUT=`which timeout` VERSION=`git describe` ./run.sh

backend=${1:-sq3}
long=$2

dir=${backend}
if [ "x$long" = "x--long" ]; then
	longopt="--long"
	dir=${dir}_long
fi

loglevel=1
nblobs=100

port=$((RANDOM + 32766))
exp=blobgen

## Each backend should provide the following functions:
#  ${backend}_prepare:	to prepare the backend and output the PID of daemons that were started, if relevant
#  ${backend}_params: 	giving the specific parameters for the oml2-server
#  ${backend}_extract:	to extract data from the storage backend as HEXs

## Sqlite3 functions
sq3_prepare() {
	dir=$1
	exp=$2
	rm -f ${dir}/${exp}.sq3
	# No PID
}
sq3_params() {
	echo "--data-dir=${dir}"
}
sq3_extract() {
	dir=$1
	exp=$2
	db=${dir}/${exp}.sq3

	seqs=$(sqlite3 $db 'SELECT oml_seq FROM blobgen_blobmp' 2>>${dir}/db.log)
	echo -n "Extracting server-stored blobs from $db:"
	for i in $seqs; do
		echo -n " $i"
		sqlite3 $db "SELECT HEX(blob) FROM blobgen_blobmp WHERE oml_seq=$i" > ${dir}/s$i.hex 2>>${dir}/db.log
	done
	echo "."
}

## PostgreSQL functions
# Some more specific variables
PGPATH=`dirname ${POSTGRES} 2>/dev/null`
PGPORT=$((RANDOM + 32766))
pg_prepare() {
	dir=$1
	exp=$2

	${PGPATH}/initdb -U oml2 ${dir}/db >> ${dir}/db.log 2>&1
	sed -i \
		-e "s/^#bytea_output *=.*/bytea_output = 'hex'/g" \
		${dir}/db/postgresql.conf
	# Outputs pg_pid for the caller
	startdaemon ${dir}/db.log "accept connections" ${POSTGRES} -k ${PWD}/${dir} -D ${PWD}/${dir}/db -p ${PGPORT}
}
pg_params() {
	echo "--backend=postgresql --pg-user=oml2 --pg-port=${PGPORT}"
}
pg_extract() {
	dir=$1
	exp=$2
	db=${exp}

	PSQL_OPTS="-h localhost -p $PGPORT $db oml2 -P tuples_only=on"
	seqs=$(${PGPATH}/psql ${PSQL_OPTS} -c 'SELECT oml_seq FROM blobgen_blobmp' 2>>${dir}/db.log) 

	echo -n "Extracting server-stored blobs from postgresql://localhost:$PGPORT $db:"
	for i in $seqs; do
		echo -n " $i"
		echo -n `${PGPATH}/psql ${PSQL_OPTS} -c "SELECT blob FROM blobgen_blobmp WHERE oml_seq=$i" 2>>${dir}/db.log | \
			sed "/^$/d;y/abcdef/ABCDEF/;s/ *\\\\\x//"` > ${dir}/s$i.hex
	done
	echo "."
}

## Start a daemon and wait for a pattern to appear in its log, or exit
# startdaemon LOGFILE PATTERN DAEMON ARGS...
startdaemon() {
	log=$1
	shift
	pattern=$1
	shift
	prog=$(basename $1)
	rest="$@"
	# stdout to somewhere, otherwise the function doesn't return;
	# the logfile is the best place, but this assumes the application
	# itself doesn't try to write there
	$rest >$log 2>&1 &
	pid=$!
	echo -n " $prog=$pid" >&2
	sleep 1
	i=0
	while ! grep -q "$pattern" "$log" ; do
			echo -n "." >&2
		if ! kill -0 ${pid} 2>/dev/null; then
			echo "DEAD" >&2
			exit 1
		elif [ $((i++)) -gt 10 ]; then
			echo "Giving up" >&2
			exit 1
		fi
		sleep 1
	done
	echo $pid
}

## Get some memory stats from OML logs
# memstats OMLVERSION EXPTYPE BACKEND path/to/SIDE.log
# Columns are: oml version, expid, side, allocated overall, freed, current; all in Bytes
memstats () {
	omlver=$1
	exptype=$2
	backend=$3
	file=$4
	side=$(basename $file .log)
	outfile=$side$exptype$backend.csv
	echo "Version,Type,Backend,Side,Allocated,Freed,Leaked,Maximum" > $outfile
	sed -n "/currently allocated/s/.*\[\([0-9]\+\).*, \([0-9]\+\).*, \([0-9]\+\).*, \([0-9]\+\).*\].*/$omlver,$exptype,$backend,$side,\1,\2,\3,\4/p" $file >> $outfile
	tail -n 1 $outfile
}

## Do the real work below
echo -n "Blob test $dir (logs in ${PWD}/$dir/):"

rm -rf $dir
mkdir $dir

# Prepare DB backend
pids=`${backend}_prepare $dir $exp`
backendparams=`${backend}_params $dir $exp`

# Start server
server_pid=`startdaemon ${dir}/server.log "EventLoop" ${top_builddir}/server/oml2-server \
	-d $loglevel --logfile - -l $port $backendparams`
echo # Finish the line

# Start client
cd ${dir}
if [ ! -z "${TIMEOUT}" ]; then
	TIMEOUT="${TIMEOUT} 30s"
else
	echo "timeout(1) utility not found; this test might hang indefinitely" >&2
fi
${TIMEOUT} ../blobgen -h -n $nblobs $longopt \
	--oml-id a --oml-domain ${exp} --oml-collect localhost:$port --oml-bufsize 110000 \
	--oml-log-level $loglevel --oml-log-file client.log
ret=$?
if [ ! $ret = 0 ]; then
	if [ $ret = 124 ]; then
		echo "Timeout generating blobs";
	else
		echo "Error $ret generating blobs";
	fi
	kill -9 $server_pid $pids
	exit $ret
fi
cd ..

# Stop oml2-server
sleep 10
echo -n "Terminating oml2-server ($server_pid)"
kill $server_pid
while kill -0 $server_pid 2>/dev/null; do echo -n '.'; sleep 1; done # Wait for the oml2-server to have exited properly
echo

# Extract data 
${backend}_extract $dir $exp

# Stop everything else
if [ -n "$pids" ]; then
	kill $pids
fi

# Calculate the diffs, return result
echo -n "Checking that server stored blobs match client-generated blobs:"
fail=0
for f in ${dir}/g*.hex; do
	g=$(basename $f .hex)
	s=$(echo $g | sed "s/g/s/")
	if [ ! -f $dir/$g.hex ]; then
		echo -n " FAIL[$((++fail))]:!$g"
	elif [ ! -f $dir/$s.hex ]; then
		echo -n " FAIL[$((++fail))]:!$s"
	elif diff -qw $dir/$g.hex $dir/$s.hex >/dev/null 2>&1; then
		echo -n " $g==$s"
	else
		echo -n " FAIL[$((++fail))]:$g!=$s"
		
	fi
done
echo "."

# Cleanup
killall -9 $server_pid $pids 2>/dev/null

memstats "$VERSION" "$exp$long" "$backend" "$dir/client.log" >> memstats.csv
memstats "$VERSION" "$exp$long" "$backend" "$dir/server.log" >> memstats.csv

exit $fail
