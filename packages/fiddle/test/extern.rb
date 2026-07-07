# Compile-time Fiddle: the Importer `extern` DSL. A module that
# `extend Fiddle::Importer` and declares `extern "C signature"` is lowered so
# `Module.func(...)` calls the C symbol directly (resolved by the linker).
# `dlload nil` binds against the already-linked image (libc/libm).
#
# This is valid CRuby, but `dlload nil` + `extern` does not resolve libc symbols
# under macOS ruby's two-level namespace, so this .expected is authored from
# values cross-validated against the equivalent Fiddle::Function.new form (which
# CRuby runs on every platform): abs(-7)=7, strlen("hello")=5, sqrt(16.0)=4.0,
# toupper(97)=65 ('a'->'A'), atoi("123")=123.
require "fiddle"
require "fiddle/import"

module Libc
  extend Fiddle::Importer
  dlload nil
  extern "int abs(int)"
  extern "size_t strlen(const char *)"
  extern "double sqrt(double)"
  extern "int toupper(int)"
  extern "int atoi(const char *)"
end

puts Libc.abs(-7)
puts Libc.strlen("hello")
puts Libc.sqrt(16.0)
puts Libc.toupper(97)
puts Libc.atoi("123")
