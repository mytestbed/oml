#
# Copyright (c) 2012 National ICT Australia (NICTA), Australia
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
# = oml4r-simple-example.rb
#
# == Description
#
# A very simple straightforward example of OML4R.
#
require 'oml4r'

# Define your own Measurement Points
class SinMP < OML4R::MPBase
  name :sin
  #channel :default

  param :label
  param :angle, :type => :int32
  param :value, :type => :double
end

class CosMP < OML4R::MPBase
  name :cos
  # channel :ch1
  # channel :default

  param :label
  param :value, :type => :double
end

# Initialise the OML4R module for your application
opts = {:appName => 'oml4rSimpleExample',
  :expID => 'foo', :nodeId => 'n1',
  :omlServer => 'file:-'} # Server could also be tcp:host:port
#  OML4R::create_channel(:default, 'file:-')    

OML4R::init(ARGV, opts)

# Now collect and inject some measurements
5.times do |i|
  sleep 0.5
  angle = 15 * i
  SinMP.inject("label_#{angle}", angle, Math.sin(angle))
  CosMP.inject("label_#{angle}", Math.cos(angle))    
end

# Don't forget to close when you are finished
OML4R::close()

# vim: sw=2
