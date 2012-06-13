#
# Copyright (c) 2010 National ICT Australia (NICTA), Australia
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
# = oml4r-wlanconfig.rb
#
# == Description
#
# This is a simple example on how to use the OML4R module to create a simple
# ruby application that "wraps" around the existing command "wlanconfig".
# This wrapper invokes "wlanconfig" at regular interval, captures and formats
# its output (= measurements), and finally pass them to OML4R, which will in 
# turn either store them in a local file or forward them to the OML Server.
#
require "oml4r"

APPNAME = "wlanconfig"
APPPATH = "/sbin/wlanconfig"
APPVERSION = "1.0"

#
# This class defines the Measurement Point for our application and the
# corresponding metrics we would like to capture
#
class MyMeasurementPoint < OML4R::MPBase
  name :wlanstat
  param :addr
  param :aid, :type => :long
  param :channel, :type => :long
  param :rate
  param :rssi, :type => :long
  param :dbm, :type => :long
  param :idle, :type => :long
  param :txseq, :type => :long
  param :rxseq, :type => :long
  # Note: other metrics potentially returned by wlanconfig are 
  # not supported here, as they are seldom set by wlanconfig.
  # These are: caps, acaps, erp, mode
end

#
# This class is the Wrapper around the existing "wlanconfig" application
#
class Wrapper

  #
  # Initialise a new Wrapper object
  # - args = the command line argument which was given to this wrapper 
  #          application
  #
  def initialize(args)

    # Initialise some variable specific to this wrapper
    @interface = nil
    @interval = 1

    # Now call the Init of OML4R with the command line arguments (args)
    # and a block defining the arguments specific to this wrapper
    OML4R::init(args) { |argParser|
      argParser.banner = "\nExecute a wrapper around #{APPNAME}\n" +
	"Use -h or --help for a list of options\n\n" 
      argParser.on("-i", "--interface IFNAME", "Name of Interface to monitor") { |name| @interface = name }
      argParser.on("-s", "--sampling DURATION", "Interval in second between sample collection for OML") { |time| @interval = time }
      argParser.on_tail("-v", "--version", "Show the version\n") { |v| puts "Version: #{APPVERSION}"; exit }
    }

    # Finally do some checking specific to this wrapper
    # e.g. here we do not proceed if the user did not give us a 
    # valid interface to monitor
    unless @interface != nil
      raise "You did not specify an interface to monitor! (-i option)"
    end
  end

  #  
  # Start the wrapped "wlaconfig" application, capture and process its output
  #
  def start()
    # Loop until the user interrupts us
    while true
      # Run the wlanconfig command
      cmd = "#{APPPATH} #{@interface} list"
      output = `#{cmd}`
      # Process its output
      processOutput(output)
      # Wait for a given duration and loop again
      sleep @interval.to_i
    end
  end

  #
  # Process each output coming from an executing of the "wlaconfig" application
  # - output =  a String holding the output to process
  #
  def processOutput(output)
    # wlanconfig returns a sequence of lines
    # The 1st line is a list of labels for the fields of the remaining lines
    # Each remaining line is for a given station, and follow the format:
    # ADDR AID CHAN RATE RSSI DBM IDLE TXSEQ RXSEQ CAPS ACAPS ERP STATE MODE
    lines = output.split("\n")
    labels = lines.delete_at(0)
    lines.each { |row|
      column = row.split(" ")
      # Inject the measurements into OML 
      MyMeasurementPoint.inject("#{column[0]}", column[1], column[2],
				"#{column[3]}", column[4], column[5],
				column[6], column[7], column[8])
    }
  end


end

#
# Entry point to this Ruby application
#
begin
  app = Wrapper.new(ARGV)
  app.start()
rescue SystemExit
rescue SignalException
  puts "Wrapper stopped."
rescue Exception => ex
  puts "Error - When executing the wrapper application!"
  puts "Error - Type: #{ex.class}"
  puts "Error - Message: #{ex}\n\n"
  # Uncomment the next line to get more info on errors
  # puts "Trace - #{ex.backtrace.join("\n\t")}"
end

# vim: sw=2
