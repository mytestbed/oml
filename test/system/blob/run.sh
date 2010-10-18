#!/bin/sh

port=4004
exp=blobgen_exp

[ -f ${exp}.sq3 ] && rm -f ${exp}.sq3
[ -f blobgen-server.log ] && rm -f blobgen-server.log
${top_builddir}/server/oml2-server -l $port -d4 --logfile=blobgen-server.log &
server_pid=$!
echo SERVER=${server_pid}

sleep 1

blobgen=./blobgen

if [ ! -x ${blobgen} ]; then
	echo "Could not find test blob generator \'${blobgen}\'"
	exit 1
fi

$blobgen -n 100 --oml-id a --oml-exp-id ${exp} --oml-server localhost:$port || exit 1

echo "Blob generating client finished OK"
sleep 1
kill $server_pid
echo "Analyzing blobs"

# Convert blobs to hex
for i in g*.bin; do
	printf "\rConverting binary blobs: $i   "
	${srcdir}/bin2hex.rb $i
done

printf "\n...done\n"

# Grab blobs from sqlite3
${srcdir}/fromsq3.sh ${exp}

# Calculate the diffs, produce result
${srcdir}/diff.sh
