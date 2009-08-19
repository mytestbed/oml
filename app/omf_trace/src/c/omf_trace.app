
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

  a.defMeasurement("radiotap") do |m|
      m.defMetric('tsft', 'long', ' wireless Timer Syncronisation Function')
      m.defMetric('rate', 'long', ' Wireless Rate')
      m.defMetric('freq', 'long', ' Wireless Channel Frequency')
      m.defMetric('sig_strength_dBm', 'long', ' Wireless Signal Strength in dB')
      m.defMetric('noise_strength_dBm', 'long', ' Wireless Noise Strength in dBm')
      m.defMetric('sig_strength', 'long', ' Wireless Signal Strength in dB')
      m.defMetric('noise_strength', 'long', ' Wireless Noise Strength in dB')
      m.defMetric('attenuation', 'long', ' Transmit Attenuation')
      m.defMetric('attenuation_dB', 'long', ' Transmit Attenuation in dB')
      m.defMetric('power', 'long', ' Transmit Power in dBm')
      m.defMetric('antenna', 'long', ' Wireless Antenna')
      m.defMetric('sourceMAC', 'long', ' Source MAC Address')
      m.defMetric('dstMAC', 'long', ' Destination MAC Address')
 end

  a.defMeasurement("ip") do |m|
    m.defMetric('ip_tos', 'long', ' Type of Service')
    m.defMetric('ip_len', 'long', ' Total Length')
    m.defMetric('ip_id', 'long', ' Identification')
    m.defMetric('ip_off', 'long', ' IP Fragment offset (and flags)')
    m.defMetric('ip_ttl', 'long', ' Time to Live')
    m.defMetric('ip_proto', 'long', 'Protocol')
    m.defMetric('ip_sum', 'long', 'Checksum')
    m.defMetric('ip_src', 'string', ' Source Address')
    m.defMetric('ip_dst', 'string', ' Destination Address')
  end

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
