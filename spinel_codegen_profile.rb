#!/usr/bin/env ruby
# CRuby-only Vernier wrapper for spinel_codegen.rb. Kept in a separate
# file so `require 'vernier'` never enters spinel's self-host pipeline
# -- spinel_codegen.rb itself stays pure spinel-subset Ruby and the
# bootstrap fixpoint is unaffected.
#
# Usage: ruby spinel_codegen_profile.rb --profile <out.json> [...spinel_codegen args]
#
# All non --profile arguments are forwarded verbatim to spinel_codegen,
# including its own --stats=csv / --stats=json flags. The Vernier
# profile is written to the --profile path after codegen exits.

profile_path = nil
forward_argv = []
i = 0
while i < ARGV.length
  if ARGV[i] == "--profile" && i + 1 < ARGV.length
    profile_path = ARGV[i + 1]
    i += 2
  else
    forward_argv << ARGV[i]
    i += 1
  end
end

if profile_path.nil?
  $stderr.puts "Usage: ruby spinel_codegen_profile.rb --profile <out.json> [...args]"
  exit(1)
end

begin
  require 'vernier'
rescue LoadError
  $stderr.puts "vernier gem not available; install with: gem install vernier"
  exit(1)
end

ARGV.replace(forward_argv)
codegen_path = File.expand_path('spinel_codegen.rb', __dir__)

Vernier.profile(out: profile_path) do
  load codegen_path
end

$stderr.puts "Vernier profile written to #{profile_path}"
