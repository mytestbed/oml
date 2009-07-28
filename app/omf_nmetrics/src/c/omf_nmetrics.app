
defApplication('omf:app:nmetrics', 'omf_nmetrics') do |a|

  a.version(1, 0)
  a.revision = 1
  a.shortDescription = 'Monitoring node statistcs'
  a.description = %{
'nmetrics' is monitoring various node specific statistics,
such as CPU, memory and network usage and reports them through 
OML. }

  a.defProperty('cpu', 'Report cpu usage', ?c, 
		:type => :flag, :impl => { :var_name => 'report_cpu' })
  a.defProperty('interface', 
		'Report usage for the specified network interface (can \
be used multiple times)', ?i, 
		:type => :string, 
		:impl => { :var_name => 'if_name', :popt_val => 'i' })
  a.defProperty('memory', 'Report memory usage', ?m, 
		:type => :flag, :impl => { :var_name => 'report_memory' })
  a.defProperty('sample-interval', 
		'Time between consecutive measurements [sec]', ?s, 
		:type => :int, :unit => 'seconds', :default => 1,
		:impl => { :var_name => 'sample_interval' })

  a.defMeasurement("memory") do |m|
    m.defMetric('ram', 'long')
    m.defMetric('total', 'long')
    m.defMetric('used', 'long')
    m.defMetric('free', 'long')
    m.defMetric('actual_used', 'long')
    m.defMetric('actual_free', 'long')
  end

  a.defMeasurement("cpu") do |m|
    m.defMetric('user', 'long')
    m.defMetric('sys', 'long')
    m.defMetric('nice', 'long')
    m.defMetric('idle', 'long')
    m.defMetric('wait', 'long')
    m.defMetric('irq', 'long')
    m.defMetric('soft_irq', 'long')
    m.defMetric('stolen', 'long')
    m.defMetric('total', 'long')
  end

  a.defMeasurement("network") do |m|
    m.defMetric('name', 'string')
    m.defMetric('rx_packets', 'long')
    m.defMetric('rx_bytes', 'long')
    m.defMetric('rx_errors', 'long')
    m.defMetric('rx_dropped', 'long')
    m.defMetric('rx_overruns', 'long')
    m.defMetric('rx_frame', 'long')
    m.defMetric('tx_packets', 'long')
    m.defMetric('tx_bytes', 'long')
    m.defMetric('tx_errors', 'long')
    m.defMetric('tx_dropped', 'long')
    m.defMetric('tx_overruns', 'long')
    m.defMetric('tx_collisions', 'long')
    m.defMetric('tx_carrier', 'long')
    m.defMetric('speed', 'long')
  end

  a.defMeasurement("procs") do |m|
    m.defMetric('cpu_id', 'long')
    m.defMetric('total', 'long')
    m.defMetric('sleeping', 'long')
    m.defMetric('running', 'long')
    m.defMetric('zombie', 'long')
    m.defMetric('stopped', 'long')
    m.defMetric('idle', 'long')
    m.defMetric('threads', 'long')
  end

  a.defMeasurement("proc") do |m|
    m.defMetric('pid', 'long')
    m.defMetric('start_time', 'long')
    m.defMetric('user', 'long')
    m.defMetric('sys', 'long')
    m.defMetric('total', 'long')
  end

  a.path = "/usr/bin/nmetrics"
end

# Local Variables:
# mode:ruby
# End:
