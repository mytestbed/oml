# -*- encoding: utf-8 -*-
require File.expand_path('../lib/oml4r.rb', __FILE__)

Gem::Specification.new do |gem|
  gem.authors       = ["NICTA"]
  gem.email         = ["oml-user@lists.nicta.com.au"]
  gem.description   = ["Simple OML client library for Ruby"]
  gem.summary       = ["This is a simple client library for OML which does not use liboml2 and its filters, but connects directly to the server using the +text+ protocol.  User can use this library to create ruby applications which can send measurement to the OML collection server."]
  gem.homepage      = "http://oml.mytestbed.net"

  gem.files         = `git ls-files`.split($\)
  gem.executables   = gem.files.grep(%r{^bin/}).map{ |f| File.basename(f) }
  gem.test_files    = gem.files.grep(%r{^(test|spec|features)/})
  gem.name          = "oml4r"
  gem.require_paths = ["lib"]
  gem.version       = OML4R::VERSION

end

# vim: sw=2
