# ClassVariableOrWriteNode -- `@@x ||= val`.
#
# Cvar version of LocalVariable/Global/Instance OrWrite. Same
# C-truthy vs Ruby-truthy caveat (numeric 0 is C-falsy but
# Ruby-truthy) -- this test stays in the agreement region.

class Cache
  @@hits = 5      # explicit non-zero literal -> folded into static decl

  def self.bump
    @@hits ||= 100   # already 5 (truthy in C and Ruby) -- no-op
  end

  def self.reset
    @@hits ||= 99    # not zero, so this is a no-op too
  end

  def self.value
    @@hits
  end
end

puts Cache.value       # 5  (folded init from class body)
Cache.bump
puts Cache.value       # 5  (||= didn't fire because 5 was truthy)
Cache.reset
puts Cache.value       # 5

# Default-init via ||= for a class-method that may run before any
# explicit write. The cvar enters the program at the type default
# (0) since there's no class-body literal init -- the first ||=
# in `init` is then the first observable value.
class Lazy
  def self.init
    @@n ||= 42
    @@n
  end
end

puts Lazy.init         # 42
puts Lazy.init         # 42 (||= no-op, value persists)
