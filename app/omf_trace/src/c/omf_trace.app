
defApplication('max:app:omf_trace', 'omf_trace') do |a|

  a.version(1, 0, 0)
  a.shortDescription = 'A short description'
  a.description = %{
A longer description describing in more detail what this application 
is doing and useful for.
}

  a.defProperty('loop', 'Create periodic result', ?l, 
		:type => :flag, :impl => { :var_name => 'loop' })
  a.defProperty('delay', 'Delay between consecutive measurements', ?d, 
		:type => :int, :unit => 'seconds',
		:impl => { :var_name => 'delay' })

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
