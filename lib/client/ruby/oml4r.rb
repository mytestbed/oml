#
# Copyright 2009-2012 National ICT Australia (NICTA), Australia
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
#
# = oml4r.rb
#
# == Description
#
# This is a simple client library for OML which does not use liboml2 and its
# filters, but connects directly to the server using the +text+ protocol.
# User can use this library to create ruby applications which can send
# measurement to the OML collection server. A simple example on how to use
# this library is attached at the end of this file. Another example can be
# found in the file oml4r-example.rb
#
require 'socket'

#
# This is the OML4R module, which should be required by ruby applications
# that want to collect measurement via OML
#
module OML4R

  VERSION = "1.0"
  REVISION = "$Revision: 1 $".split(":")[1].chomp("$").strip
  VERSION_STRING = "OML4R Client Library - Version #{VERSION} (#{REVISION})"
  DEF_SERVER_PORT = 3003

  #
  # Measurement Point Class
  # Ruby applications using this module should sub-class this MPBase class
  # to define their own Measurement Point (see the example at the end of
  # this file)
  #
  class MPBase

    # Some Class variables
    @@defs = {}
    @@frozen = false
    @@useOML = false
    @@start_time = nil

    # Execute a block for each defined MP
    def self.each_mp(&block)
      @@defs.each &block
    end

    # Set the useOML flag
    def self.__useOML__()
      @@useOML = true
    end

    # Returns the definition of this MP
    def self.__def__()
      unless (defs = @@defs[self])
	defs = @@defs[self] = {}
	defs[:p_def] = []
	defs[:seq_no] = 0
      end
      defs
    end

    # Set a name for this MP
    def self.name(name)
      __def__()[:name] = name
    end

    # Set a metric for this MP
    # - name = name of the metric to set
    # - opts = a Hash with the options for this metric
    #          Only supported option is :type = { :string | :long | :double }
    def self.param(name, opts = {})
      o = opts.dup
      o[:name] = name
      o[:type] ||= :string
      __def__()[:p_def] << o
      nil
    end

    # Inject a measurement from this Measurement Point to the OML Server
    # However, if useOML flag is false, then only prints the measurement on stdout
    # - args = a list of arguments (comma separated) which correspond to the
    #          different values of the metrics for the measurement to inject
    def self.inject(*args)
      # If the OML parameters were not set, then do not inject to a OML server
      # But instead print the measurement on stdout
      unless @@useOML
        line = ""
        args.each { |arg|
          line << "#{arg} "
        }
        puts "#{line}"
        return
      end
      # Check that the list of values passed as argument matches the
      # definition of this Measurement Point
      defs = __def__()
      pdef = defs[:p_def]
      if args.size != pdef.size
        raise "OML4R: Size mismatch between the measurement and the MP definition!"
      end

      # Now prepare the measurement...
      a = []
      a << Time.now - @@start_time
      a << defs[:index]
      a << (defs[:seq_no] += 1)
      args.each do |arg|
	a << arg
      end
      # ...and inject it!
      @@sout.puts a.join("\t")
    end

    def self.start_time()
      @@start_time
    end

    # Freeze the definition of further MPs
    # - sout = the output stream to the OML collection server
    def self.__freeze__(sout)
      @@frozen = true
      @@sout = sout
      @@start_time = Time.now
    end

    # Build the table schema for this MP and send it to the OML collection server
    # - name_prefix = the name for this MP to use as a prefix for its table
    def self.__print_meta__(name_prefix = nil)
      return unless @@frozen
      sout = @@sout
      defs = __def__()

      # Do some sanity checks...
      unless (mp_name = defs[:name])
	raise "Missing 'name' declaration for '#{self}'"
      end
      unless (name_prefix.nil?)
	mp_name = "#{name_prefix}_#{mp_name}"
      end
      unless (index = defs[:index])
	raise "Missing 'index' declaration for '#{self}'"
      end

      # Build the schema and send it
      out = ['schema:', index, mp_name]
      defs[:p_def].each do |d|
	out << "#{d[:name]}:#{d[:type]}"
      end
      sout.puts out.join(' ')
    end
  end # class MPBase

  #
  # The Init method of OML4R
  # Ruby applications should call this method to initialise the OML4R module
  # This method will parse the command line argument of the calling application
  # to extract the OML specific parameters, it will also do the parsing for the
  # remaining application-specific parameters.
  # It will then connect to the OML server (if requested on the command line), and
  # send the initial instruction to setup the database and the tables for each MPs.
  #
  # - argv = the Array of command line arguments from the calling Ruby application
  # - & block = a block which defines the additional application-specific arguments
  #
  def self.init(argv, opts = {}, &block)

    expID = opts[:expID]
    nodeID = opts[:nodeID]
    appID = opts[:appID]
    omlServer = opts[:omlServer]
    omlPort = opts[:omlPort]
    omlFile = opts[:omlFile]

    # Create a new Parser for the command line
    require 'optparse'
    opts = OptionParser.new
    # Include the definition of application's specific arguments
    yield(opts) if block
    # Include the definition of OML specific arguments
    opts.on("--oml-expid EXPID", "Experiment ID for OML") { |name| expID = name }
    opts.on("--oml-nodeid NODEID", "Node ID for OML") { |name| nodeID = name }
    opts.on("--oml-appid APPID", "Application ID for OML") { |name| appID = name }
    opts.on("--oml-server SERVER", "Address to the OML Collection Server") { |name| omlServer = name }
    opts.on("--oml-port PORT", "PORT to the OML Collection Server") { |name| omlPort = name }
    opts.on("--oml-file FILENAME", "Filename for local storage of measurement") { |name| omlFile = name }
    # Now parse the command line
    rest = opts.parse(argv)

    if !omlServer && !omlFile && !ENV['OML_SERVER'].nil?
      transport, target, port = ENV['OML_SERVER'].split(':')
      case transport
      when 'tcp'
	omlServer = target
	omlPort = port
      when 'file'
	omlFile = target
      else
	puts "OML4R: Unknown transport '#{transport}'"
      end
    end
    expID ||= ENV['OML_EXP_ID']
    nodeID ||= ENV['OML_NAME']

    if !omlServer && !omlFile
      puts "OML4R: OML disabled."
      return rest
    else
      MPBase.__useOML__()
      puts "OML4R: OML enabled."
    end

    # Now connect to the OML Server
    if (host = omlServer)
      port = omlPort || DEF_SERVER_PORT
      sout = TCPSocket.new(host, port)
    # Or open a local file for storage
    elsif (fname = omlFile)
      sout = fname == '-' ? $stdout : File.open(fname, "w+")
    else
      raise 'OML4R: Missing server url or path to local file!'
    end
    unless expID && nodeID && appID
      raise 'OML4R: Missing values for parameters expID, nodeID, or appID!'
    end

    # Handle the defined Measurement Points
    MPBase.__freeze__(sout)

    # Finally, send initial OML commands to server (or local file)
    sout.puts "protocol: 1"
    sout.puts "experiment-id: #{expID}"
    sout.puts "start_time: #{MPBase.start_time().tv_sec}"
    sout.puts "sender-id: #{nodeID}"
    sout.puts "app-name: #{appID}"
    sout.puts "content: text"

    i = 1
    MPBase.each_mp do |klass, defs|
      defs[:index] = i
      klass.__print_meta__(appID)
      i += 1
    end
    sout.puts ""

    rest || []
  end

end # module OML4R

#
# A very simple straightforward example
# Also look at the file oml4r-example.rb for another simple
# example.
#
if $0 == __FILE__

  # Define your own Measurement Point
  class MyMP < OML4R::MPBase
    name :sin
    param :label
    param :angle, :type => :long
    param :value, :type => :double
  end

  # Initialise the OML4R module for your application
  args = ["--oml-expid", "foo",
	  "--oml-nodeid", "n1",
	  "--oml-appid", "app1",
	  "--oml-file", "myLocalFile.db"]
  OML4R::init(args)

  # Now collect and inject some measurements
  10.times do |i|
    angle = 15 * i
    MyMP.inject("label_#{angle}", angle, Math.sin(angle))
    sleep 1
  end
end
