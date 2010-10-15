
builddir=$1
port=3004
exp=blobgen_exp

[ -f ${exp}.sq3 ] && rm -f ${exp}.sq3
[ -f blobgen-server.log ] && rm -f blobgen-server.log
$builddir/server/oml2-server -l $port -d4 --logfile=blobgen-server.log &
server_pid=$!
echo SERVER=${server_pid}

sleep 1

blobgen=$builddir/test/system/blob/blobgen

$blobgen -n 100 --oml-id a --oml-exp-id ${exp} --oml-server localhost:$port

echo "Ran blob generator"
sleep 1
kill $server_pid
echo "Analyzing blobs"

# Convert blobs to hex
for i in g*.bin; do
	./bin2hex.rb $i
done

# Grab blobs from sqlite3
./fromsq3.sh ${exp}

# Calculate the diffs, produce result
./diff.sh
