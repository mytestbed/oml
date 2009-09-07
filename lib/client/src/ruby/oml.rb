#
# Copyright (c) 2009 National ICT Australia (NICTA), Australia
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
# = oml.rb
#
# == Description
#
# This is a simple client library for OML which does not use liboml2 and its
# filters, but connects directly to the server using the +text+ protocol.
#
require 'socket'

module OML

  VERSION = "1.0"
  REVISION = "$Revision: 1 $".split(":")[1].chomp("$").strip
  VERSION_STRING = "OML Client Library - Version #{VERSION} (#{REVISION})"

  DEF_SERVER_PORT = 3001

  class MPBase
    @@defs = {}
    @@frozen = false

    def self.each_mp(&block)
      @@defs.each &block
    end

    def self.__def__()
      unless (defs = @@defs[self])
	defs = @@defs[self] = {}
	defs[:p_def] = []
	defs[:seq_no] = 0
      end
      defs
    end

    def self.name(name)
      __def__()[:name] = name
    end

    def self.param(name, opts = {})
      o = opts.dup
      o[:name] = name
      o[:type] ||= :string
      __def__()[:p_def] << o
      nil
    end

    def self.inject(*args)
      defs = __def__()
      pdef = defs[:p_def]
      raise "Parameter count mismatch" if args.size != pdef.size

      a = []
      a << Time.now - @@start_time
      a << defs[:index]
      a << (defs[:seq_no] += 1)
      args.each do |arg|
	a << arg
      end

      @@sout.puts a.join("\t")
    end

    def self.__freeze__(sout)
      @@frozen = true
      @@sout = sout
      @@start_time = Time.now
    end

    def self.__print_meta__(name_prefix = nil)
      return unless @@frozen

      sout = @@sout
      defs = __def__()

      unless (mp_name = defs[:name])
	raise "Missing 'name' declaration for '#{self}'"
      end
      unless (name_prefix.nil?)
	mp_name = "#{name_prefix}_#{mp_name}"
      end

      unless (index = defs[:index])
	raise "Missing 'index' declaration for '#{self}'"
      end
      out = ['schema:', index, mp_name]
      defs[:p_def].each do |d|
	out << "#{d[:name]}:#{d[:type]}"
      end
      sout.puts out.join(' ')
    end
  end # class MPBase

  def self.init(opts)
    exp_id = opts[:exp_id]
    node_id = opts[:node_id]
    app_name = opts[:app_name]

    if (host = opts[:server_name])
      port = opts[:server_port] || DEF_SERVER_PORT
      sout = TCPSocket.new(host, port)
    elsif (fname = opts[:file])
      sout = fname == '-' ? $stdout : File.open(fname, "w+")
    else
      raise 'Missing server url'
    end

    unless exp_id && node_id && app_name
      raise 'Missing options'
    end

    sout.puts "protocol: 1"
    sout.puts "experiment-id: #{exp_id}"
    sout.puts "start_time: #{Time.now.tv_sec}"
    sout.puts "sender-id: #{node_id}"
    sout.puts "app-name: #{app_name}"
    sout.puts "content: text"


    MPBase.__freeze__(sout)
    i = 1
    MPBase.each_mp do |klass, defs|
      defs[:index] ||= i
      klass.__print_meta__(app_name)
      i += 1
    end
    sout.puts ""
  end

end # module OML

if $0 == __FILE__
  
  class MyMP < OML::MPBase
    name :sin
    param :label
    param :angle, :type => :long
    param :value, :type => :double
  end

  OML::init(:exp_id => 'foo', :node_id => 'n1', :app_name => 'app1',
	    :server_name => 'localhost')

  10.times do |i|
    angle = 15 * i
    MyMP.inject("lab_#{angle}", angle, Math.sin(angle))
    sleep 1
  end

end
