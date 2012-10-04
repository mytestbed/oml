#!/bin/sh

PGPATH=`dirname ${POSTGRES}`

port=4004
pgport=5433
exp=blobgen_exp
db=${exp}
long=$1
dir=
if [ "x$long" = "x--long" ]; then
	dir=longpg
else
	dir=shortpg
fi
echo -n "Blob tests for PostgreSQL backend ($dir):"

rm -rf $dir ${dir}_db
mkdir -p $dir

${PGPATH}/initdb -U oml2 ${dir}_db >> ${dir}/pg.log 2>&1
sed -i "s/^#bytea_output *=.*/bytea_output = 'hex'/g" ${dir}_db/postgresql.conf
${POSTGRES} -D ${PWD}/${dir}_db -p $pgport >> ${dir}/pg.log 2>&1 &
pg_pid=$!
echo -n " postgresql PID=${pg_pid}"
sleep 1
${PGPATH}/createdb -U oml2 -p $pgport ${db}

${top_builddir}/server/oml2-server -d 4 -l $port --logfile=${dir}/blobgen-server.log --backend=postgresql --pg-user=oml2 --pg-port=5433 &
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
	kill $pg_pid;
	kill $server_pid;
	exit 1
fi
cd ..

sleep 1
kill $server_pid

# Grab blobs
PSQL_OPTS="-p $pgport $db oml2 -P tuples_only=on"
seqs=$(${PGPATH}/psql ${PSQL_OPTS} -c 'SELECT oml_seq FROM blobgen_blobmp') 

echo -n "Extracting server-stored blobs from $db:"
for i in $seqs; do
	echo -n " $i"
	echo -n `${PGPATH}/psql ${PSQL_OPTS} -c "SELECT blob FROM blobgen_blobmp WHERE oml_seq=$i" | sed "/^$/d;y/abcdef/ABCDEF/;s/ *\\\\\x//"` > ${dir}/s$i.hex
done
echo "."

# Calculate the diffs, produce result
${srcdir}/diff.sh ${dir}
status=$?

sleep 1
kill $pg_pid

exit $status
