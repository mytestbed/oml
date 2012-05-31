
#
# This little example triggers a bug in the OML server by sending an empy string 
# as the last parameter

require 'oml4r'

class BugMP < OML4R::MPBase
  name :bug
  param :index, :type => :long
  param :label
end

unless url = ARGV[0]
  puts "usage: #{$0} oml_url"
  exit
end

opts = { :domain => 'test', :nodeID => "n1", :appID => 'bug', :omlURL => url }
OML4R::init(ARGV, opts)

BugMP.inject 1, 'OK'
BugMP.inject 2, ''

OML4R::close