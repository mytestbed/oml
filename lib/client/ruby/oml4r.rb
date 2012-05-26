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
    @@channels = {}
    @@frozen = false
    @@useOML = false
    @@start_time = nil

    # Execute a block for each defined MP
    def self.each_mp(&block)
      @@defs.each &block
    end

    # Set the useOML flag. If set to false, make 'inject' a NOOP
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
    
    # Set the channel these measurements should be sent out on.
    # Multiple declarations are allowed, and ':default' identifies
    # the channel defined by the command line arguments or environment variables.
    #
    def self.channel(channel)
      (@@channels[self] ||= []) << channel
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
      return unless @@useOML

      # Check that the list of values passed as argument matches the
      # definition of this Measurement Point
      defs = __def__()
      pdef = defs[:p_def]
      if args.size != pdef.size
        raise "OML4R: Size mismatch between the measurement and the MP definition!"
      end

      # Now prepare the measurement...
      t = Time.now - @@start_time
      a = []
      a << (defs[:seq_no] += 1)
      args.each do |arg|
        a << arg
      end
      # ...and inject it!
      msg = a.join("\t")
      @@channels[self].each do |ca|
        channel = ca[0]
        index = ca[1]
        channel.send "#{t}\t#{index}\t#{msg}"
      end
    end

    def self.start_time()
      @@start_time
    end

    # Freeze the definition of further MPs
    #
    def self.__freeze__(appID, start_time)
      @@frozen = true
      # replace channel names with channel object
      self.each_mp do |klass, defs|
        cna = @@channels[klass] || []
        ca = cna.collect do |name|
          # return it in an array as we need to add the channel specific index  
          [Channel[name.to_sym]]
        end
        #puts "Using channels '#{ca.inspect}"
        @@channels[klass] = ca.empty? ? [[Channel[]]] : ca
      end
      @@start_time = start_time
      
      #i = 1
      self.each_mp do |klass, defs|
        #defs[:index] = i
        klass.__print_meta__(appID)
        #i += 1
      end
      
    end

    # Build the table schema for this MP and send it to the OML collection server
    # - name_prefix = the name for this MP to use as a prefix for its table
    def self.__print_meta__(name_prefix = nil)
      return unless @@frozen
      defs = __def__()

      # Do some sanity checks...
      unless (mp_name = defs[:name])
	       raise "Missing 'name' declaration for '#{self}'"
      end
      unless (name_prefix.nil?)
	      mp_name = "#{name_prefix}_#{mp_name}"
      end
      
      @@channels[self].each do |ca|
        #puts "Setting up channel '#{ca.inspect}"
        index = ca[0].send_schema(mp_name, defs[:p_def])
        ca << index
      end
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

    expID = opts[:expID] || ENV['OML_EXP_ID']
    nodeID = opts[:nodeID] || ENV['OML_NAME']
    appID = opts[:appID]
    omlUrl = ENV['OML_URL']
    noop = false

    # Create a new Parser for the command line
    require 'optparse'
    opts = OptionParser.new
    # Include the definition of application's specific arguments
    yield(opts) if block
    # Include the definition of OML specific arguments
    opts.on("--oml-expid EXPID", "Experiment ID for OML [#{expID || 'undefined'}]") { |name| expID = name }
    opts.on("--oml-nodeid NODEID", "Node ID for OML [#{nodeID || 'undefined'}]") { |name| nodeID = name }
    opts.on("--oml-appid APPID", "Application ID for OML [#{appID || 'undefined'}]") { |name| appID = name }
    opts.on("--oml-server SERVER", "URL for the OML Server (tcp:host:port)") { |u|  omlUrl = u }
    opts.on("--oml-file FILENAME", "Filename for local storage of measurement") { |name| omlUrl = "file:#{name}" }
    opts.on("--oml-noop", "Do not collect measurements") { noop = true }    

    opts.on_tail("-h", "--help", "Show this message") { puts opts; exit }

    # Now parse the command line
    rest = opts.parse(argv)
    return if noop

    Channel.create(:default, omlUrl) if omlUrl
    
    unless expID && nodeID && appID
      raise 'OML4R: Missing values for parameters expID, nodeID, or appID!'
    end
    
    # Handle the defined Measurement Points
    startTime = Time.now
    Channel.init_all(expID, nodeID, appID, startTime)
    MPBase.__freeze__(appID, startTime)

    MPBase.__useOML__()
    puts "OML4R: OML enabled."

    rest || []
  end
  
  #
  # Measurement Point Class
  # Ruby applications using this module should sub-class this MPBase class
  # to define their own Measurement Point (see the example at the end of
  # this file)
  #
  class Channel
    @@channels = {}
    
    def self.create(name, url)
      if channel = @@channels[name]
        if url != channel.url
          raise "OML4R: Channel '#{name}' already defined with different url"
        end
        return channel
      end

      #oml_opts = {:exp_id => 'image_load', :node_id => 'n1', :app_name => 'img_load'}
      if url.start_with? 'file:'
        proto, fname = url.split(':')
        out = (fname == '-' ? $stdout : File.open(fname, "w+"))
      elsif url.start_with? 'tcp:'
        #tcp:norbit.npc.nicta.com.au:3003
        proto, host, port = url.split(':')
        port ||= DEF_SERVER_PORT
        out = TCPSocket.new(host, port)
      else
        raise "OML4R: Unknown transport in server url '#{url}'"
      end
      @@channels[name] = self.new(url, out)      
    end
    
    def self.[](name = :default)
      unless (@@channels.key?(name = name.to_sym))
        raise "OML4R: Unknown channel '#{name}'"
      end
      @@channels[name]
    end
    
    def self.init_all(expID, nodeID, appID, startTime)
      @@channels.values.each { |c| c.init(expID, nodeID, appID, startTime) }
    end
    
    attr_reader :url
    
    def send_schema(mp_name, pdefs) # defs[:p_def]
      # Build the schema and send it
      @index += 1
      line = ['schema:', @index, mp_name]
      pdefs.each do |d|
        line << "#{d[:name]}:#{d[:type]}"
      end
      msg = line.join(' ')
      send msg
      @index
    end
      
    def send(msg)
      @out.puts msg
    end
  
    def init(expID, nodeID, appID, startTime)
      send_protocol_header(expID, nodeID, appID, startTime)
    end

    protected
    
    def initialize(url, out_stream)
      @url = url
      @out = out_stream
      @index = 0
    end

    
    def send_protocol_header(expID, nodeID, appID, startTime)
      @out.puts "protocol: 1"
      @out.puts "experiment-id: #{expID}"
      @out.puts "start_time: #{startTime.tv_sec}"
      @out.puts "sender-id: #{nodeID}"
      @out.puts "app-name: #{appID}"
      @out.puts "content: text"
    end
  end # Channel

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
    #channel :default
    
    param :label
    param :angle, :type => :long
    param :value, :type => :double
  end

  # Define your own Measurement Point
  class MyMP2 < OML4R::MPBase
    name :cos
    channel :ch1
    channel :default

    param :label
    param :value, :type => :double
  end

  puts "Check 'test.db' for outputs as well"
  OML4R::Channel.create(:ch1, 'file:test.db')
  
  # Initialise the OML4R module for your application
  args = ["--oml-expid", "foo",
	  "--oml-nodeid", "n1",
	  "--oml-appid", "app1",
#	  "--oml-file", "-"
	  ]
  OML4R::Channel.create(:default, 'file:-')	  
	  
  OML4R::init(args)

  # Now collect and inject some measurements
  10.times do |i|
    angle = 15 * i
    MyMP.inject("label_#{angle}", angle, Math.sin(angle))
    MyMP2.inject("lebal_#{angle}", Math.cos(angle))    
    sleep 1
  end
end
