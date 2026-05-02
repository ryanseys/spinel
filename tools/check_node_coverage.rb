#!/usr/bin/env ruby
# frozen_string_literal: true
#
# tools/check_node_coverage.rb — Spinel Prism node coverage audit
#
# Cross-references the canonical Prism node list (vendor/prism/config.yml)
# against (a) the parser's switch in spinel_parse.c and (b) the codegen's
# string-typed dispatches in spinel_codegen.rb. Reports gaps.
#
# Run via `make check-coverage` or `ruby tools/check_node_coverage.rb`.
# Exits zero if every Prism node is handled OR documented in
# tools/excluded_nodes.txt; nonzero otherwise. Suitable for CI.
#
# This script runs under CRuby (not the Spinel-compilable subset), so it
# uses YAML and standard idioms freely.

require "yaml"
require "set"

ROOT = File.expand_path("..", __dir__)
CONFIG_YML = File.join(ROOT, "vendor", "prism", "config.yml")
PARSER_C = File.join(ROOT, "spinel_parse.c")
CODEGEN_RB = File.join(ROOT, "spinel_codegen.rb")
EXCLUDED_TXT = File.join(ROOT, "tools", "excluded_nodes.txt")

abort "Prism config.yml not found at #{CONFIG_YML}" unless File.exist?(CONFIG_YML)
abort "spinel_parse.c not found"  unless File.exist?(PARSER_C)
abort "spinel_codegen.rb not found" unless File.exist?(CODEGEN_RB)

# ---- Load the canonical node list from Prism's config.yml ----

config = YAML.load_file(CONFIG_YML)
prism_nodes = config.fetch("nodes").map { |n| n.fetch("name") }.sort

# ---- Discover which nodes the parser handles ----
# Pattern: `case PM_FOO_BAR_NODE:` → "FooBarNode".

parser_src = File.read(PARSER_C)
parser_pm_constants = parser_src.scan(/\bcase\s+PM_([A-Z_]+_NODE):/).flatten.uniq

def pm_constant_to_node_name(const)
  const.split("_").map(&:capitalize).join.then { |s| s.sub(/Node\z/, "Node") }
end

parser_handled = parser_pm_constants.map { |c| pm_constant_to_node_name(c) }.sort

# Cross-check: every PM_*_NODE the parser uses must map to a real Prism node.
parser_unknown = parser_handled - prism_nodes
unless parser_unknown.empty?
  warn "WARNING: parser references nodes not in config.yml: #{parser_unknown.join(', ')}"
end

# ---- Discover which nodes the codegen dispatches on ----
# Pattern: `if t == "FooBarNode"` (or `if @nd_type[...] == "FooBarNode"`).

codegen_src = File.read(CODEGEN_RB)
codegen_dispatched = codegen_src
  .scan(/"([A-Z][A-Za-z]*Node)"/)
  .flatten
  .uniq
  .select { |n| prism_nodes.include?(n) }
  .sort

# ---- Load the documented exclusion list ----

excluded = Set.new
if File.exist?(EXCLUDED_TXT)
  File.foreach(EXCLUDED_TXT) do |line|
    line = line.strip
    next if line.empty? || line.start_with?("#")
    name, _rationale = line.split(/\s*#\s*/, 2)
    excluded << name
  end
end

# ---- Compute gaps ----

parser_missing = prism_nodes - parser_handled
codegen_missing = prism_nodes - codegen_dispatched

# ---- Report ----

prism_version = "(unknown)"
prism_changelog = File.join(ROOT, "vendor", "prism", "CHANGELOG.md")
if File.exist?(prism_changelog)
  first_version = File.foreach(prism_changelog).find { |l| l =~ /^## v?\d+\.\d+\.\d+/ }
  prism_version = first_version.to_s.strip if first_version
end

puts "Spinel Prism Node Coverage Report"
puts "================================="
puts "Prism: #{prism_version}"
puts "Total nodes in config.yml: #{prism_nodes.size}"
puts

puts "Parser coverage (spinel_parse.c)"
puts "  Handled: #{parser_handled.size}"
puts "  Missing: #{parser_missing.size}"
parser_missing.each do |n|
  marker = excluded.include?(n) ? "[excluded]" : "[GAP]"
  puts "    #{marker} #{n}"
end
puts

puts "Codegen coverage (spinel_codegen.rb dispatches)"
puts "  Dispatched (top-level or referenced as string literal): #{codegen_dispatched.size}"
puts "  Not dispatched: #{codegen_missing.size}"
puts "  (Note: some are reachable via parent walking — Group B in the plan.)"
puts

real_gaps = parser_missing - excluded.to_a
if real_gaps.empty?
  puts "RESULT: OK — every node is handled or in the documented exclusion list."
  exit 0
else
  puts "RESULT: GAPS — #{real_gaps.size} nodes are missing and not in tools/excluded_nodes.txt:"
  real_gaps.each { |n| puts "  - #{n}" }
  puts
  puts "Either implement these in spinel_parse.c, or add them to tools/excluded_nodes.txt"
  puts "with a one-line rationale."
  exit 1
end
