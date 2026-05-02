# SourceEncodingNode — the `__ENCODING__` keyword.
#
# CRuby returns an Encoding object; Spinel has no Encoding runtime,
# so __ENCODING__ returns the canonical name as a string. All Spinel
# sources are assumed to be UTF-8. We compare against to_s for parity.

puts __ENCODING__.to_s
