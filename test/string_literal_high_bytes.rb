# Encode->decode round-trip for a string literal containing bytes >= 0x80.
# Mirrors lib/sow/img/png.rb's PNG signature, which currently must be
# expressed as `[0x89,...].pack("C*")` to avoid tripping the parser's
# UTF-8-naive string emit path. After this fix, the raw `"\x89PNG..."`
# form parses + analyzes cleanly.
MAGIC = "\x89PNG\r\n\x1A\n"
raise "bytesize mismatch" unless MAGIC.bytesize == 8
raise "bad first byte" unless MAGIC.bytes.first == 0x89
puts "ok"
