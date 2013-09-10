#!/usr/bin/env python

# A test to see if self-instrumentation of connection/disconections is
# working as expected. The client opens a text-protocol connection to
# an OML server and then sends a header before terminating the
# connection.
#
# \see http://oml.mytestbed.net/projects/oml/wiki/Description_of_Text_protocol
#
# Author: Steve Glass
#
# Date: 21/08/2013
#

import argparse
import sys
import os
import socket
from time import time
from time import sleep


# Compatibility conversion of Python 2 and 3's string type
#
if float(sys.version[:3])<3:
    def to_bytes(s):
        return s
else:
    def to_bytes(s):
        return bytes(s, "UTF-8")
# Python3's range used to be Python2's xrange
#
try:
    xrange
except NameError:
    xrange = range

# Perform a client connection selftest
#
def selftest():

    """
    This is an automated server instrumentation selftest for OML.
    """

    # process the command line
    parser = argparse.ArgumentParser()
    parser.add_argument("--oml-id", default="selftest", help="node identifier")
    parser.add_argument("--oml-domain", default="server-inst", help="experimental domain")
    parser.add_argument("--oml-collect", default="localhost:3003", help="URI for a remote collection point")
    parser.add_argument("--timeout", type=int, default=2, help="time to sleep before disconnecting")
    parser.add_argument("--count", type=int, default=3, help="number of connection attempts")
    args = parser.parse_args()

    # parse collection URI
    try:

        uri = args.oml_collect.split(":")
        if len(uri) == 1:
            # host
            server = uri[0]
            port = DEFAULT_PORT
        elif len(uri) == 2:
            if uri[0] == "tcp":
                # tcp:host
                server = uri[1]
                port = self.DEFAULT_PORT
            else:
                # host:port
                server = uri[0]
                port = int(uri[1])
        elif len(uri) == 3:
            # tcp:host:port
            server = uri[1]
            port = int(uri[2])
        else:
            sys.stderr.write("ERROR\tMalformed collection URI \n" % self.omlport)
            exit(1)

    except ValueError:
        sys.stderr.write("ERROR\tCannot use '%s' as a port number\n" % self.omlport)
        exit(1)

    # connect, send header and disconnect
    for i in xrange(0, args.count):
        try:

            # establish a connection
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(5) 
            sock.connect((server, port))
            sock.settimeout(None)

            # create and send header
            header = "protocol: 4\n"
            header += "domain: " + args.oml_domain + "\n"
            header += "start-time: " + str(time()) + "\n"
            header += "sender-id: " + str(args.oml_id) + "\n"
            header += "app-name: server-inst\n"
            header += "schema: 0 _experiment_metadata subject:string key:string value:string\n"
            header += "content: text\n\n"
            sock.send(to_bytes(header))

            # wait a little while
            sleep(args.timeout)

            # disconnect
            sock.close()

        except socket.error as e:
            sys.stderr.write("ERROR\tCould not connect to OML server: %s\n" %  e)
            exit(2)
        except Exception:
            x = sys.exc_info()[1]
            sys.stderr.write("ERROR\tUnexpected " + str(x) + "\n")
            exit(3)


if __name__ == '__main__':
    selftest()


# Local Variables:
# mode: Python
# indent-tabs-mode: nil
# tab-width: 4
# python-indent: 4
# End:
# vim: sw=4:sts=4:expandtab
