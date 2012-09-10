# 
# Description: Python Class for OML Connection and Data Uploading
#
# From http://oml.mytestbed.net/projects/oml/wiki/Description_of_Text_protocol
#
# The client opens a TCP connection to an OML server and then sends a header 
# followed by a sequence of measurement tuples. The header consists of a 
# sequence of key, value pairs terminated by a new line. The end of the header 
# section is identified by an empty line. Each measurement tuple consists of 
# a new-line-terminated string which contains TAB-separated text-based 
# serialisations of each tuple element. Finally, the client ends the session 
# by simply closing the TCP connection.
#
# Author: Fraida Fund
#
# Date: 08/06/2012
#

import os
import socket
from time import time
from time import sleep


class OMLBase:
    """
    This is a Python OML class
    """
    def __init__(self,appname,expid=None,sender=None,uri=None):

        self.oml = True

        # Set all the instance variables
        self.appname = appname
        if self.appname[:1].isdigit() or '-' in self.appname or '.' in self.appname:
          print "OML::Invalid app name: %s" %  self.appname
          self.oml = False

        if expid is None:
          try:
            self.expid = os.environ['OML_EXP_ID']
          except KeyError:
            self.oml = False
        else:
          self.expid = expid

        if uri is None:
          try:
            uri_l = os.environ['OML_SERVER'].split(":")
          except KeyError:
            self.oml = False
        else:
          uri_l = uri.split(":")

        try:
          self.omlserver = uri_l[1]
          self.omlport = int(uri_l[2])
        except NameError:
          self.oml = False
        except IndexError:
          self.oml = False

        if sender is None:
          try:
            self.sender =  os.environ['OML_NAME']
          except KeyError:
            self.oml = False
        else:
          self.sender = sender

        self.starttime = 0

        if self.oml:        
          # Set socket
          self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
          self.sock.settimeout(5) 
          self.streams = 0
          self.schema = ""
          self.nextseq = {}
          self.streamid = {}


    def addmp(self,mpname,schema):
 
      if self.oml:
        if "-" in mpname or "." in mpname:
          print "OML::Invalid measurement point name: %s" %  mpname
          self.oml = False

        else:
          # Count number of streams
          self.streams += 1

          if self.streams > 1:
             self.schema += '\n'

          str_schema = "schema: " + str(self.streams) + " " + self.appname + "_" + mpname + " " + schema

          self.schema += str_schema
          self.nextseq[mpname] = 0
          self.streamid[mpname] = self.streams
   

    def start(self):

      if self.oml:        
        # Use socket to connect to OML server
        try:
          self.sock.connect((self.omlserver,self.omlport))
          self.sock.settimeout(None)
  
          self.starttime = int(time())

          header = "protocol: 1" + '\n' + "experiment-id: " + str(self.expid) + '\n' + \
                 "start_time: " + str(self.starttime) + '\n' + "sender-id: " + str(self.sender) + '\n' + \
                 "app-name: " + str(self.appname) + '\n' + \
                 str(self.schema) + '\n' + "content: text" + '\n' + '\n'    
    
          self.sock.send(header)
          print "OML::Connected to %s:%d" % (self.omlserver,self.omlport)
        except socket.error, e:
          print "OML::Error on connect to OML server: %s" %  e
          self.oml = False
          print "OML::OML Disabled"
      else:
        print "OML::OML Disabled"


    def close(self):
      streamid = None

      if self.oml:
        self.sock.close()


    def inject(self,mpname,values):

      if self.oml:        
        # Inject data into OML 

        if self.starttime == 0:
            print("OML::Did not call OML start")
            self.oml = False
        else:
          timestamp = time() - self.starttime
          try:
            streamid = self.streamid[mpname]
            seqno = self.nextseq[mpname]
            str_inject = str(timestamp) + '\t' + str(streamid) + '\t' + str(seqno)

            try:
              for item in values:
                str_inject += '\t'
                str_inject += str(item)
              str_inject += '\n'
              self.sock.send(str_inject)
              self.nextseq[mpname]+=1
            except TypeError:
              print "OML::Invalid measurement list."
          except KeyError:
            print "OML::Tried to inject into unknown MP %s" % mpname
    
      else:
        timestamp = time() - self.starttime
        str_inject = str(timestamp) + '\t'
        try:
          for item in values:
              str_inject += '\t'
              str_inject += str(item)
          print str_inject
        except TypeError:
          print "OML::Invalid measurement list."

