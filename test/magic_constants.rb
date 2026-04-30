# __FILE__, __dir__, __LINE__ magic constants. Lower to compile-time
# string / int literals threaded from spinel_parse:
# - __FILE__ uses the as-given path (relative or absolute, no symlink
#   resolution) — matches CRuby
# - __dir__ uses the realpath-canonicalized dirname — matches CRuby's
#   File.realpath(File.dirname(__FILE__))
# - __LINE__ uses Prism's per-node line metadata (already worked)

# __FILE__ ends with the source basename
puts __FILE__.end_with?("test/magic_constants.rb")

# __dir__ ends with "/test" (the test/ directory under spinel/)
puts __dir__.end_with?("/test")

# __LINE__ at known offsets — these line numbers must match the
# physical line position in this file
puts __LINE__
puts __LINE__
puts __LINE__

# Length of __FILE__ is non-zero
puts __FILE__.length > 0

# Length of __dir__ is non-zero
puts __dir__.length > 0

# __FILE__ contains "magic_constants"
puts __FILE__.include?("magic_constants")
