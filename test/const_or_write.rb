# ConstantOrWriteNode -- `FOO ||= val`.
#
# Lowered at parser side to: if !FOO; FOO = val; end (modulo
# defined?-vs-truthiness semantics; CRuby actually checks definedness
# on the read, not just truthiness, but for already-initialized
# constants the truthiness check matches).

CONFIGURED = "explicit"
CONFIGURED ||= "fallback"  # truthy -> doesn't fire
puts CONFIGURED            # explicit

# Numeric initialized to non-zero
TIMEOUT = 30
TIMEOUT ||= 60
puts TIMEOUT               # 30
