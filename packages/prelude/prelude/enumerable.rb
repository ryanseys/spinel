# Core prelude: Enumerable methods implemented in Ruby (CRuby prelude.rb
# style). Spliced implicitly by the compiler when the program calls one of
# these names on a custom Enumerable class (see the prelude registry in
# src/spinel_parse.c) -- or pulled explicitly with
# `require "prelude/enumerable"`.
#
# These attach via an ordinary `module Enumerable` reopen, so they serve
# custom classes that `include Enumerable`; native receivers (typed arrays,
# ranges, hashes) keep their native arms, and a user definition later in
# the document wins over the prelude.
#
# House rules for prelude code (current engine constraints):
#   - call builtin/sibling methods with an explicit `self.` receiver
#   - snapshot the receiver with `each` (bare `to_a` does not resolve here)
#   - block-form only for yielding methods (a blockless call of a
#     block_given?-branching method loses the early-return value through
#     the inliner)
#   - avoid names the native chain emitters key on (zip, reverse_each, ...)

module Enumerable
  # cycle { } / cycle(n) { }: repeat the snapshot forever (a block break is
  # the only exit) or n times. The blockless Enumerator form stays a loud
  # compile-time reject, like the unbounded blockless Array#cycle.
  def cycle(n = nil)
    a = []
    each { |x| a << x }
    return nil if a.empty?
    if n.nil?
      while true
        a.each { |x| yield x }
      end
    else
      k = 0
      while k < n
        a.each { |x| yield x }
        k += 1
      end
    end
    nil
  end
end
