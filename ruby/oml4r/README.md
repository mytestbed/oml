OML4R: Native OML Implementation in Ruby			{#oml4rdoc}
========================================

This is a simple client library for OML which does not use liboml2 and its
filters, but connects directly to the server using the text protocol [0].
User can use this library to create ruby applications which can send
measurement to the OML collection server. A simple example on how to use
this library is attached at the end of this file. Another example can be
found in the file oml4r-example.rb

Installation
------------

OML4R is available from RubyGems [1].

    $ gem install oml4r


Usage
-----

### Definition of a Measurement Point

    class MyMP < OML4R::MPBase
      name :mymp
    
      param :mystring
      param :myint, :type => :int32
      param :mydouble, :type => :double
    end

### Initialisation, Injection and Tear-down

    OML4R::init(ARGV, {
    	:appName => 'oml4rSimpleExample',
    	:expID => 'foo',
    	:nodeId => 'n1',
    	:omlServer => 'file:-'}
    )
    MyMP.inject("hello", 13, 37.)
    OML4R::close()

### Real example

See examples files oml4r-simple-example.rb and oml4r-wlanconfig.rb.


License
-------

Copyright 2009-2012 National ICT Australia (NICTA), Australia

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

[1]: http://oml.mytestbed.net/projects/oml/wiki/Description_of_Text_protocol
[2]: https://rubygems.org/gems/oml4r
