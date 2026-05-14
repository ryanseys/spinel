MAGIC = "\x89PNG\r\n\x1A\n"
raise "bytesize mismatch" unless MAGIC.bytesize == 8
raise "bad first byte" unless MAGIC.bytes.first == 0x89
puts "ok"
