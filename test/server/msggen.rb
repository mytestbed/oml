#!/usr/bin/env ruby

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

def run
  prog = ARGV[0]
  puts prog

  puts ENV['srcdir']
  puts Dir.getwd

  io = IO.popen("./msgloop", "w+")

  @test_data.each { |msg|
    io.puts msg
  }
  puts "Closed? #{io.closed?}"
  puts "Wrote all test data to msgloop pipe"

  io.close_write
  puts "Reading from pipe"
  io.readlines.each { |line| puts line }
  puts "Closed pipe; exiting"
  exit 0
end

if __FILE__ == $PROGRAM_NAME then run; end
