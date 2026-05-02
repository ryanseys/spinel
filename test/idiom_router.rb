# Idiomatic Ruby: a tiny URL router that exercises several new
# nodes (AssocSplat, InterpolatedRegex, captures, forwarding) in
# a recognizable design pattern. Output is identical under CRuby
# and Spinel, byte-for-byte.

# Default route table; each handler returns a string.
defaults = {
  index: "Welcome",
  notfound: "404"
}

# Per-request overrides merged with defaults via AssocSplat.
overrides = {index: "Welcome (admin)"}
routes = {**defaults, **overrides}

# Match a path with a captured segment via InterpolatedRegex,
# building the pattern from a configurable prefix.
prefix = "users"
def lookup(path, prefix, routes)
  if path =~ /\A\/#{prefix}\/(\d+)\z/
    "user-" + $1
  elsif path == "/"
    routes[:index]
  else
    routes[:notfound]
  end
end

puts lookup("/", prefix, routes)              #=> Welcome (admin)
puts lookup("/users/42", prefix, routes)      #=> user-42
puts lookup("/users/7", prefix, routes)       #=> user-7
puts lookup("/missing", prefix, routes)       #=> 404

# Forwarding façade -- a "log + dispatch" middleware pattern.
def real_handler(...)
  puts "handled"
end
def with_logging(...)
  puts "logging"
  real_handler(...)
end
def request(...)
  with_logging(...)
end
request
#=> logging
#=> handled
