# Mega integration test for the new node families landed this batch.
#
# Combines forwarding + InterpolatedRegex + AssocSplat + FlipFlop +
# Rational/Imaginary literals into a single program. Demonstrates that
# the new features compose correctly with each other and with the
# existing Spinel runtime.

# A logging façade that forwards arbitrary calls down to a printer.
def actually_print(...)
  puts "log: forwarded"
end
def log_forward(...)
  actually_print(...)
end
def kickoff_log(...)
  log_forward(...)
end
kickoff_log
#=> log: forwarded

# Build a config dict by merging defaults with a user override.
defaults = {host: "localhost", port: 80}
overrides = {port: 8080}
config = {**defaults, **overrides}
puts config[:host]              #=> localhost
puts config[:port].to_s         #=> 8080

# Use an interpolated regex against config-derived data.
target_host = config[:host]
url = "https://localhost/api"
if url =~ /https:\/\/#{target_host}\//
  puts "matched"                #=> matched
end

# FlipFlop filter: emit lines from index 2 through index 4 inclusive.
items = ["zero", "one", "two", "three", "four", "five"]
i = 0
while i < items.length
  if (i == 2)..(i == 4)
    puts items[i]
  end
  i = i + 1
end
#=> two
#=> three
#=> four

# Rational + Imaginary literals as data values.
ratio = 3.5r
imag = 2i
puts ratio                       #=> 7/2
puts imag                        #=> 0+2i
