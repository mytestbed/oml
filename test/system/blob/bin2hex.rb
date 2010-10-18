#!/usr/bin/env ruby

def run
  if ARGV[0].nil? then exit 1; end
  binname = ARGV[0]
  hexname = ARGV[0].sub(".bin", ".hex")

  bin = File.open(binname,"rb") { |io| io.read }
  hex = File.open(hexname, "w+")
  for i in 0..(bin.length-1) do
    hex.printf("%02X", bin[i])
  end
  hex.close
end


if __FILE__ == $PROGRAM_NAME then run; end
