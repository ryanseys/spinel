#!/usr/bin/env ruby
# CRuby-only Vernier wrapper for spinel_codegen.rb. Kept in a separate
# file so `require 'vernier'` never enters spinel's self-host pipeline
# -- spinel_codegen.rb itself stays pure spinel-subset Ruby and the
# bootstrap fixpoint is unaffected.
#
# Modes:
#   --mode wall      (default) wall-time sampling profile
#   --mode retained  retained-memory profile (Vernier.trace_retained)
#
# Wall-mode flags (ignored in retained mode):
#   --interval <us>  sampling interval in microseconds (default: 100)
#   --alloc <n>      allocation sampling interval; 0 disables (default: 10)
#   --hooks <list>   comma-separated hook names (default: memory_usage)
#
# Output:
#   --profile <path>  explicit output path (default: derived under
#                     build/profiles/<ast-basename>.<mode>.vernier.json)
#   The absolute output path is printed to STDOUT on a single line so
#   callers can capture it: out=$(ruby spinel_codegen_profile.rb ...)
#
# Examples:
#   ruby spinel_codegen_profile.rb build/codegen.ast build/codegen.ir /tmp/c
#   ruby spinel_codegen_profile.rb --mode retained build/codegen.ast ...
#   ruby spinel_codegen_profile.rb --interval 50 --alloc 5 ...
#
# Read the output without a browser via bin/analyze_profile.rb:
#   out=$(ruby spinel_codegen_profile.rb ...)
#   ruby bin/analyze_profile.rb "$out"

require 'fileutils'

mode = "wall"
interval = 100
allocation_interval = 10
hooks_arg = "memory_usage"
profile_path = nil
forward_argv = []

i = 0
while i < ARGV.length
  arg = ARGV[i]
  case arg
  when "--mode"
    mode = ARGV[i + 1]
    i += 2
  when "--interval"
    interval = Integer(ARGV[i + 1])
    i += 2
  when "--alloc"
    allocation_interval = Integer(ARGV[i + 1])
    i += 2
  when "--hooks"
    hooks_arg = ARGV[i + 1]
    i += 2
  when "--profile"
    profile_path = ARGV[i + 1]
    i += 2
  else
    forward_argv << arg
    i += 1
  end
end

unless %w[wall retained].include?(mode)
  $stderr.puts "spinel_codegen_profile: unknown --mode #{mode.inspect} (wall|retained)"
  exit 1
end

if profile_path.nil?
  ast_arg = forward_argv.find { |a| a.end_with?(".ast") }
  base = ast_arg ? File.basename(ast_arg, ".ast") : "codegen"
  profile_path = File.expand_path("build/profiles/#{base}.#{mode}.vernier.json", __dir__)
end
FileUtils.mkdir_p(File.dirname(profile_path))

begin
  require 'vernier'
rescue LoadError
  $stderr.puts "vernier gem not available."
  $stderr.puts "Install with: gem install vernier"
  $stderr.puts "Or via bundler: bundle add vernier --group=development"
  exit 1
end

ARGV.replace(forward_argv)
codegen_path = File.expand_path('spinel_codegen.rb', __dir__)

case mode
when "wall"
  hooks = hooks_arg.split(",").reject(&:empty?).map(&:to_sym)
  Vernier.profile(
    out: profile_path,
    interval: interval,
    allocation_interval: allocation_interval,
    hooks: hooks,
  ) do
    load codegen_path
  end
when "retained"
  Vernier.trace_retained(out: profile_path) do
    load codegen_path
  end
end

puts profile_path
