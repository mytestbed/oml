#!/usr/bin/env ruby
# vim: sw=2

@test_data = [
              "OML000000420000000CHello World",
              "OML000000430000000CHellp World",
              "OML000000440000000CHellq World",
              "OML000000450000000CHellr World",
              "OML000000460000000CHells World",
              "OML000000470000000CHellt World",
              "OML000000480000000CHellr World",
              "OML000000490000000CHellv World",
              "OML0000004A0000000CHellw World",
              "OML000000420000000CHellx World",
              "OML000000420000000CHelly World",
              "OML000000420000000CHellz World",
             ]

def serialize_text(value)
  value.to_s
end

def serialize(type_, value, content)
  if content == "text"
    serialize_text(value)
  end
end

class Client
  def initialize(sender, domain, application, start_time)
    @sender = sender
    @domain = domain
    @application = application
    @start_time = start_time
    @streams = []
  end

  # schema should be a hash (name => type)
  def defStream(index, name, schema)
    @streams << Stream.new(index,name,schema)
  end

  def headers(content)
    h = []
    h3 = [] # Header V3
    h4 = [] # Header V4

    h3 << "protocol: 3"
    h3 << "experiment-id: #{@domain}v3"
    h3 << "start_time: #{@start_time + 3}"
    h3 << "sender-id: #{@sender}v3"
    h3 << "app-name: #{@application}v3"
    h3 = h3 + @streams.collect { |stream| stream.schema_string }
    h3 << "content: #{content}"
    h << h3

    h4 << "protocol: 4"
    h4 << "domain: #{@domain}v4"
    h4 << "start-time: #{@start_time+4}"
    h4 << "sender-id: #{@sender}v4"
    h4 << "app-name: #{@application}v4"
    h4 << "content: #{content}"
    # Stream definitions can appears anywhere in v4
    h4 = h4 + @streams.collect { |stream| stream.schema_string }
    h << h4

    h
  end

  # Map old version of the headers to the most recent one.
  def header_map_Vn_one(h)
    [
      [ /start_time/, "start-time"],
      [ /experiment-id/, "domain" ],
    ].each do |re, repl|
      h = h.gsub(re, repl)
    end
    h
  end

  def header_map_Vn(t)
    if t.kind_of?(Array)
      t.map { |h| header_map_Vn_one(h) }
    else
      header_map_Vn_one(t)
    end
  end

  def gen(index, content)
    if index < 1 || index > @streams.length
      return
    end

    sample = @streams[index-1].gen

    if content == "text" then
      header = ["#{sample[:timestamp]}",
                "#{sample[:index]}",
                "#{sample[:seqno]}"].join("\t")
      values = @streams[index-1].schema.each_value.zip(sample[:measurement])
      measurement = values.each.collect { |t,v| serialize(t, v, content) }
      header + "\t" + measurement.join("\t") + "\n"
    end
  end
end

class Stream
  attr_reader :schema

  def initialize(index, name, schema)
    @index = index
    @name = name
    @schema = schema
    @generators = @schema.each_value.collect { |v| Generator.new(v) }
    @seqno = 0
    @timestamp = 0.0
  end

  def schema_string
    field_specs = @schema.each_pair.collect { |n, t| "#{n}:#{t}" }.join(" ")
    "schema: #{@name} #{@index} #{field_specs}"
  end

  def gen
    @seqno += 1
    @timestamp += rand
    { :index => @index,
      :seqno => @seqno,
      :timestamp => @timestamp,
      :measurement => @generators.collect { |g| g.gen } }
  end
end

def random_string(n)
  length = Math.sqrt(rand((n/2)**2)).to_i # Maybe not optimal
  length.times.collect { (rand(26) + 65).chr }.join
end

class Generator
  def initialize(oml_type)
    @type = oml_type
  end

  def gen
    val = case @type
          when :long, :int32 then rand(1 << 32) - (1 << 31)
          when :int64  then rand(1 << 64) - (1 << 63)
          when :uint32 then rand(1 << 32)
          when :uint64 then rand(1 << 64)
          when :double then rand
          when :string then random_string(254)
          end
    case @type
    when :long, :int32 then
      if val == (1 << 32) then
        val = (1 << 32)-1
      end
    when :int64 then
      if val == (1 << 64) then
        val = (1 << 64)-1
      end
    end
    val
  end

  def get(&block)
    yield gen
  end
end

def make_packet(seqno, string)
  ("OML%08x%08x" % [seqno, string.length]) + string
end

def run
  c = Client.new("a", "ae", "app", 1234556)
  c.defStream(1, "generator_lin", "label" => :string, "seq_no" => :long)
  c.defStream(2, "generator_sin", "label" => :string, "phase" => :double, "angle" => :double)

  allhdrs = c.headers("text")

  testid = 0
  fails = 0

  ndata = 20
  ntests = ndata * allhdrs.length + allhdrs.map { |h| h.length}.reduce(:+)

  # We generate TAP [0,1] compatible output
  # [0] http://szabgab.com/tap--test-anything-protocol.html
  # [1] https://www.gnu.org/software/automake/manual/automake.html#Using-the-TAP-test-protocol
  puts "1..#{ntests}"

  allhdrs.each do |headers|
    header_map = {}
    headers.each { |h|
      a = h.split(':')
      header_map[c.header_map_Vn(a[0])] = a[1..-1].join(':').strip
    }
    packets = []
    ndata.times { packets << c.gen(1, "text") }

    io = IO.popen("./msgloop", "w+")

    seqno = 0
    hstr = headers.join("\n") + "\n\n"
    io.write(make_packet(seqno, hstr)); seqno += 1
    packets.each { |p| io.write(make_packet(seqno, p)); seqno += 1}
    puts "# msggen: wrote all test data to msgloop pipe"

    io.close_write
    puts "# msggen: receiving interpreted results..."
    io.readlines.each { |line|
      res = "--"
      testid = testid + 1
      if line[0] == ?H then
	a = line.split('>')
	if c.header_map_Vn(headers).include?("#{a[1]}: #{a[2].strip}") then
	  res =  "ok"
	else
	  puts "# for '#{a[1]}'"
	  puts "# expected '#{header_map[a[1]]}'"
	  puts "# received '#{a[2].strip}'"
	  res =  "not ok"
	  fails = fails + 1
	end
      elsif line[0] == ?T then
	a = line.split('>')
	if packets.include?(a[1]) then
	  res =  "ok"
	else
	  res =  "not ok"
	  fails = fails + 1
	end
      end
      puts "#{res} #{testid} - #{line.strip}"
    }
    puts "# #{fails}/#{testid-1} tests failed so far"
  end

  exit fails
end

if __FILE__ == $PROGRAM_NAME then run; end

# vim: sw=2
