#!/bin/bash
#
# Example event hook for the OML server, copying an Sqlite database elsewhere
# when the last client has exited.
# Copyright 2012 National ICT Australia (NICTA), Australia
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
irodsUserName=rods
irodsHost=irods.example.com
#irodsPort=1247
irodsZone=tempZone
export irodsUserName irodsHost irodsPort irodsZone

LOGFILE=oml2-server-hook.log
function log ()
{
	echo "$@" >&2
	echo "$@" >> ${LOGFILE}
}

# XXX: You might need to initialise the iRODS password for the UNIX user
# running tho oml2-server by running 'iinit' to create ~/.irods/.irodsA on its
# behalf for iput to work
IPUT=iput
PGDUMP=pg_dump

echo "OML HOOK READY"
while read COMMAND ARGUMENTS; do
	# One report line must be printed in each control path;
	# this first one puts out a timestamp and a dump of the received command, but no newline
	log -n "`date`: ${COMMAND} ${ARGUMENTS}: "
	case "${COMMAND}" in
		"DBCLOSED")
			case "${ARGUMENTS}" in
				file:*)
					DBFILE=${ARGUMENTS/file:/}
					log "SQLite3 DB ${DBFILE} closed, pushing to iRODS"
					${IPUT} ${DBFILE}
					;;
				postgresql://*)
					# Separate the components of the URI by gradually eating them off the TMP variable
					TMP="${ARGUMENTS/postgresql:\/\//}"
					USER=${TMP/@*/}
					TMP=${TMP/${USER}@/}
					HOST=${TMP/:*/}
					TMP=${TMP/${HOST}:/}
					PORT=${TMP/\/*/}
					TMP=${TMP/${PORT}\//}
					DBNAME=${TMP}
					DBFILE=${DBNAME}.`date +%Y-%m-%d_%H:%M:%S%z`.pg.sql
					log "PostgreSQL DB ${DBNAME} closed, dumping as ${DBFILE} and pushing to iRODS"
					${PGDUMP} -U ${USER} -h ${HOST} -p ${PORT} ${DBNAME} > ${DBFILE}
					${IPUT} ${DBFILE}
					;;
				*)
					log "DB ${ARGUMENTS} closed, but don't know how to handle it"
					;;
			esac
			;;
		"EXIT")
			log "Exiting"
			exit 0
			;;
		*)
			log "Unknown command"
			;;
	esac
done
