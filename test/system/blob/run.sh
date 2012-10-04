#!/bin/sh

port=4004
exp=blobgen_exp
long=$1
dir=
if [ "x$long" = "x--long" ]; then
	dir=long
else
	dir=short
fi
echo -n "Blob tests for SQLite3 backend ($dir):"

mkdir -p $dir
rm -f ${dir}/*.hex # Remove leftover blob data from last run

[ -f ${dir}/${exp}.sq3 ] && rm -f ${dir}/${exp}.sq3
[ -f ${dir}/blobgen-server.log ] && rm -f ${dir}/blobgen-server.log
${top_builddir}/server/oml2-server -l $port --logfile=${dir}/blobgen-server.log --data-dir=${dir} &
server_pid=$!
echo " oml2-server PID=${server_pid}"
sleep 1

cd $dir
blobgen=../blobgen
if [ ! -x ${blobgen} ]; then
	echo "Could not find test blob generator \'${blobgen}\'"
	exit 1
fi

if [ ! -z "${TIMEOUT}" ]; then
	TIMEOUT="${TIMEOUT} 30s"
fi
${TIMEOUT} $blobgen -h -n 100 $long --oml-id a --oml-domain ${exp} --oml-collect localhost:$port --oml-bufsize 110000
if [ $? != 0 ]; then
	echo "Error or timeout generating blobs"; 
	kill $server_pid;
	exit 1
fi
cd ..

sleep 1
kill $server_pid

# Grab blobs from sqlite3
${srcdir}/fromsq3.sh ${dir}/${exp}

# Calculate the diffs, produce result
${srcdir}/diff.sh ${dir}
status=$?

exit $status
