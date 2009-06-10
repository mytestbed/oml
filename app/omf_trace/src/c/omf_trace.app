
defApplication('max:app:omf_trace', 'omf_trace') do |a|

  a.version(1, 0, 0)
  a.shortDescription = 'A short description'
  a.description = %{
A longer description describing in more detail what this application 
is doing and useful for.
}

  a.defProperty('filter', 'Filter expression BPFEXP', ?f, 
		:type => :string)
  a.defProperty('snaplen', '???', ?s, 
		:type => :int, :unit => 'bytes')
  a.defProperty('promisc', '???', ?p, 
		:type => :flag)
  a.defProperty('interface', 'Interface to trace', ?i, 
		:type => :string)




  a.defMeasurement("sensor") do |m|
    m.defMetric('val', 'long')
    m.defMetric('inverse', 'double')
    m.defMetric('name', 'string')
  end

  a.path = "/usr/local/bin/omf_trace"
end

# Local Variables:
# mode:ruby
# End:
