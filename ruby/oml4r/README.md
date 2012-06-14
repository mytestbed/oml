# OML4R

This is a simple client library for OML which does not use liboml2 and its
filters, but connects directly to the server using the text protocol.
User can use this library to create ruby applications which can send
measurement to the OML collection server. A simple example on how to use
this library is attached at the end of this file. Another example can be
found in the file oml4r-example.rb

## Installation

    $ gem install oml4r

## Usage

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

