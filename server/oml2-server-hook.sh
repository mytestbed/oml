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

# XXX: You might need to initialise the iRODS password for the UNIX user
# running tho oml2-server by running 'iinit' to create ~/.irods/.irodsA on its
# behalf for iput to work
IPUT=iput

echo "OML HOOK READY"
while read COMMAND ARGUMENTS; do
	echo "`date` '${COMMAND}' '${ARGUMENTS}'" >> oml2-server-hook.log
	case "${COMMAND}" in
		"DBCLOSED")
			case "${ARGUMENTS}" in
				file:*)
					DBFILE=${ARGUMENTS/file:/}
					echo "db ${DBFILE} closed, pushing to iRODS..." >&2
					${IPUT} ${DBFILE}
					;;
				postgresql://*)
					DBFILE=${ARGUMENTS}
					echo "Don't really know how to handle ${DBFILE}..." >&2
					;;
				*)
					echo "db ${ARGUMENTS} closed, but don't know how to handle it..." >&2
					;;
			esac
			;;
		"EXIT")
			echo "exiting..." >&2
			exit 0
			;;
		*)
			;;
	esac
done
