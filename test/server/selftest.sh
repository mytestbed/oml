#!/bin/bash

# Server instrumentation selftest
#
# This script should be run by user with permissions to
# read/write/delete DBDIR. 

TESTDIR=`dirname $0`
OMLDIR=(`cd ${TESTDIR}/../..; pwd`)
DBDIR="${TESTDIR}/testdb"
SQLDB="${DBDIR}/selftest.sq3"

# Clean up DB
[ -e "$DBDIR" ] && rm -fr "$DBDIR"
mkdir -p "$DBDIR"

port=$((RANDOM + 32766))

# Start an OML server
DOMAIN=selftest
${OMLDIR}/server/oml2-server -l $port -D "$DBDIR" --oml-collect localhost:$port --oml-domain "$DOMAIN" --oml-id oml2-server > server.log 2>&1 &
OMLPID=$!

# Wait for server to begin then connect/disconnect
sleep 5
${TESTDIR}/selftest.py --oml-collect localhost:$port

# Wait a bit more and terminate server
sleep 20
kill -TERM $OMLPID
wait $OMLPID

# Now check table was written
if [ -e "${SQLDB}" ]; then

	 COUNT="${TESTDIR}/rowcount"
	 sqlite3 "${SQLDB}" <<EOF
.output ${COUNT}
SELECT COUNT(*) FROM server_clients;
EOF
	 if [[ $? = 0 && -e "$COUNT" ]]; then
		  n=`cat ${COUNT}`
		  if [ 0 -lt $n ]; then
				echo 2>&1 "PASS:	server instrumentation"
		  else
				echo 2>&1 "ERROR:	no (dis)connection entries!"
				exit 1
		  fi
	 else
		  echo 2>&1 "ERROR:	Failed to query results from ${SQLDB}!"
		  exit 1
	 fi

else
	 echo 2>&1 "ERROR:	Failed to find ${SQLDB}!"
	 exit 2
fi

# Remove DB and other files
[ -e "$DBDIR" ] && rm -fr "$DBDIR"
[ -e "$COUNT" ] && rm -f "$COUNT"

exit 0
