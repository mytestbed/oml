# -*- encoding: utf-8 -*-
require File.expand_path('../lib/oml4r.rb', __FILE__)

Gem::Specification.new do |gem|
  gem.authors       = ["NICTA"]
  gem.email         = ["oml-user@lists.nicta.com.au"]
  gem.description   = ["Simple OML client library for Ruby"]
  gem.summary       = ["This is a simple client library for OML which does not use liboml2 and its filters, but connects directly to the server using the +text+ protocol.  User can use this library to create ruby applications which can send measurement to the OML collection server."]
  gem.homepage      = "http://oml.mytestbed.net"

  # ls-files won't work in VPATH builds;
  # plus, we want lib/oml4r.rb rather than lib/oml4r.rb.in
  #gem.files         = `git ls-files`.split($\)
  gem.files	    = [
	  ".yardopts",
	  "Gemfile",
	  "LICENSE",
	  "README.md",
	  "Rakefile",
	  "lib/oml4r.rb",
	  "lib/oml4r/oml4r-simple-example.rb",
	  "lib/oml4r/oml4r-wlanconfig.rb",
	  "oml4r.gemspec",
  ]
  gem.executables   = gem.files.grep(%r{^bin/}).map{ |f| File.basename(f) }
  gem.test_files    = gem.files.grep(%r{^(test|spec|features)/})
  gem.name          = "oml4r"
  gem.require_paths = ["lib"]
  gem.version       = OML4R::VERSION

end

# vim: sw=2
