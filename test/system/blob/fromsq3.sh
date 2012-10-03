#!/bin/sh
sq3=$1
db=$sq3.sq3
dir=$(dirname $sq3)
seqs=$(sqlite3 $db 'SELECT oml_seq FROM blobgen_blobmp')

echo -n "Extracting server-stored blobs from $db:"
for i in $seqs; do
	echo -n " $i"
	sqlite3 $db "SELECT HEX(blob) FROM blobgen_blobmp WHERE oml_seq=$i" > ${dir}/s$i.hex
done
echo "."
