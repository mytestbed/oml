#!/bin/sh

echo=/bin/echo
sq3=$1
db=$sq3.sq3
seqs=$(sqlite3 $db 'SELECT oml_seq FROM blobgen_blobmp')

printf "Extracting server-stored blobs from $db\n"

for i in $seqs; do
	printf "\r-->$i  "
	sqlite3 $db "SELECT HEX(blob) FROM blobgen_blobmp WHERE oml_seq=$i" > s$i.hex
done

printf "\n...done\n"
