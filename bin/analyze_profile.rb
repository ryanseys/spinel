#!/usr/bin/env ruby
# Programmatic analyzer for Vernier .vernier.json[.gz] profiles
# (Firefox Profiler format). Emits a markdown report with the top
# functions by self time, total time, allocation count, and (in
# retained mode) retained bytes -- no profiler.firefox.com required.
#
# Usage:
#   ruby bin/analyze_profile.rb path/to/profile.vernier.json [--out report.md]
#   ruby bin/analyze_profile.rb --diff baseline.json after.json [--out diff.md]
#
# Pure stdlib (json + zlib). No external gems.

require "json"
require "zlib"

TOP_SELF   = 30
TOP_TOTAL  = 30
TOP_ALLOC  = 20
TOP_RETAIN = 20

def load_profile(path)
  raw = if path.end_with?(".gz")
          Zlib::GzipReader.open(path, &:read)
        else
          File.read(path)
        end
  JSON.parse(raw)
end

# Walk every (stack, weight) pair in samples / jsAllocations and yield to the
# block. Used for both self/total time and allocation accumulation.
def each_sample(table)
  return unless table
  stacks  = table["stack"]
  weights = table["weight"]
  return unless stacks && weights
  size = table["length"] || stacks.size
  i = 0
  while i < size
    yield stacks[i], weights[i] if stacks[i]
    i += 1
  end
end

# For a stack index, walk the stackTable.prefix chain and yield each func_idx.
# The first yielded func_idx is the LEAF (self), the rest are callers (total).
def walk_stack(stack_idx, stack_prefix, stack_frame, frame_func)
  cur = stack_idx
  while cur
    frame_idx = stack_frame[cur]
    yield frame_func[frame_idx]
    cur = stack_prefix[cur]
  end
end

# Build per-function rollups from a thread's data. Returns:
#   {func_idx => {self: w, total: w, alloc: w, retained: w}}
# `mode` indicates how to interpret weights (:samples vs :bytes).
def accumulate(thread)
  func_data = Hash.new { |h, k| h[k] = { self: 0, total: 0, alloc: 0, retained: 0 } }

  stack_table = thread["stackTable"]
  frame_table = thread["frameTable"]
  return func_data unless stack_table && frame_table

  stack_prefix = stack_table["prefix"]
  stack_frame  = stack_table["frame"]
  frame_func   = frame_table["func"]

  samples = thread["samples"]
  is_retained = samples && samples["weightType"] == "bytes"

  each_sample(samples) do |stack_idx, weight|
    seen = {}
    leaf = true
    walk_stack(stack_idx, stack_prefix, stack_frame, frame_func) do |fi|
      if leaf
        func_data[fi][:self] += weight
        leaf = false
      end
      unless seen[fi]
        if is_retained
          func_data[fi][:retained] += weight
        else
          func_data[fi][:total] += weight
        end
        seen[fi] = true
      end
    end
  end

  each_sample(thread["jsAllocations"]) do |stack_idx, weight|
    seen = {}
    leaf = true
    walk_stack(stack_idx, stack_prefix, stack_frame, frame_func) do |fi|
      func_data[fi][:alloc] += weight if leaf
      leaf = false
      seen[fi] = true
    end
  end

  func_data
end

# Format a markdown table from rows (array of arrays), with the given header.
def format_table(header, rows)
  return "(empty)\n" if rows.empty?
  widths = header.each_with_index.map { |h, i| [h.length, *rows.map { |r| r[i].to_s.length }].max }
  fmt = ->(r) { "| " + r.each_with_index.map { |c, i| c.to_s.ljust(widths[i]) }.join(" | ") + " |" }
  sep = "| " + widths.map { |w| "-" * w }.join(" | ") + " |"
  ([fmt.call(header), sep] + rows.map(&fmt)).join("\n") + "\n"
end

# Resolve func_idx -> displayable {name, file, line} via stringArray indices.
def resolver_for(thread)
  strings   = thread["stringArray"]
  func_tbl  = thread["funcTable"]
  names     = func_tbl["name"]
  filenames = func_tbl["fileName"]
  lines     = func_tbl["lineNumber"]
  ->(fi) do
    name = strings[names[fi]] || "?"
    file = filenames[fi] ? strings[filenames[fi]] : nil
    line = lines[fi]
    loc = file ? (line ? "#{file}:#{line}" : file) : "-"
    [name, loc]
  end
end

# Compute total wall duration in ms from samples.time (vernier's
# meta.interval is hardcoded to 1 in the firefox output — FIXME upstream).
def duration_ms_from_samples(thread)
  samples = thread["samples"]
  return 0.0 unless samples && samples["time"] && !samples["time"].empty?
  samples["time"].max - samples["time"].min
end

# Render one profile to markdown. Returns the string.
def render_profile(path, profile)
  threads = profile["threads"] || []

  out = String.new
  out << "# Profile: #{File.basename(path)}\n\n"
  out << "- mode: #{infer_mode(profile)}\n"
  out << "- threads: #{threads.size}\n"

  main = threads.find { |t| t["isMainThread"] } || threads.first
  return out + "(no thread data)\n" unless main

  duration_ms = duration_ms_from_samples(main)
  out << "- duration: #{('%.2f' % (duration_ms / 1000.0))} s\n\n"

  func_data = accumulate(main)
  resolve = resolver_for(main)
  total_self = func_data.values.sum { |v| v[:self] } || 0
  total_total = func_data.values.sum { |v| v[:total] } || 0
  total_alloc = func_data.values.sum { |v| v[:alloc] } || 0
  total_retained = func_data.values.sum { |v| v[:retained] } || 0

  # Weights in wall mode are sample-counts (vernier run-length-encodes
  # adjacent identical stacks). Convert to ms by proportion of the total
  # captured duration.
  weight_to_ms = ->(w) {
    total_self.zero? ? 0.0 : (w.to_f / total_self * duration_ms)
  }

  out << "- samples: #{main['samples'] ? main['samples']['length'] : 0}\n"
  out << "- unique stacks: #{main['stackTable'] ? main['stackTable']['length'] : 0}\n\n"

  # Self-time table
  if total_self > 0
    rows = func_data.sort_by { |_, v| -v[:self] }.first(TOP_SELF).map do |fi, v|
      name, loc = resolve.call(fi)
      pct = (100.0 * v[:self] / total_self).round(2)
      [pct, weight_to_ms.call(v[:self]).round(1), weight_to_ms.call(v[:total]).round(1), name, loc]
    end
    out << "## Top #{TOP_SELF} by self time\n\n"
    out << format_table(%w[% self_ms total_ms function file:line], rows)
    out << "\n"
  end

  # Total-time table
  if total_total > 0
    rows = func_data.sort_by { |_, v| -v[:total] }.first(TOP_TOTAL).map do |fi, v|
      name, loc = resolve.call(fi)
      pct = (100.0 * v[:total] / total_total).round(2)
      [pct, weight_to_ms.call(v[:total]).round(1), weight_to_ms.call(v[:self]).round(1), name, loc]
    end
    out << "## Top #{TOP_TOTAL} by total time\n\n"
    out << format_table(%w[% total_ms self_ms function file:line], rows)
    out << "\n"
  end

  # Allocations table
  if total_alloc > 0
    rows = func_data.sort_by { |_, v| -v[:alloc] }.first(TOP_ALLOC).map do |fi, v|
      name, loc = resolve.call(fi)
      pct = (100.0 * v[:alloc] / total_alloc).round(2)
      [pct, v[:alloc], name, loc]
    end
    out << "## Top #{TOP_ALLOC} allocation sites (alloc_count proxies sampled-alloc weight)\n\n"
    out << format_table(%w[% alloc_weight function file:line], rows)
    out << "\n"
  end

  # Retained table
  if total_retained > 0
    rows = func_data.sort_by { |_, v| -v[:retained] }.first(TOP_RETAIN).map do |fi, v|
      name, loc = resolve.call(fi)
      pct = (100.0 * v[:retained] / total_retained).round(2)
      [pct, format_bytes(v[:retained]), name, loc]
    end
    out << "## Top #{TOP_RETAIN} retained sites\n\n"
    out << format_table(%w[% retained function file:line], rows)
    out << "\n"
  end

  out << render_counters(profile)

  out
end

def infer_mode(profile)
  threads = profile["threads"] || []
  main = threads.find { |t| t["isMainThread"] } || threads.first
  return "unknown" unless main && main["samples"]
  main["samples"]["weightType"] == "bytes" ? "retained" : "wall"
end

def format_bytes(n)
  units = %w[B KiB MiB GiB TiB]
  i = 0
  v = n.to_f
  while v >= 1024 && i < units.size - 1
    v /= 1024
    i += 1
  end
  "#{('%.2f' % v)} #{units[i]}"
end

# Render memory_usage hook counter as a text sparkline.
def render_counters(profile)
  counters = profile["counters"] || []
  return "" if counters.empty?
  out = String.new
  counters.each do |counter|
    name = counter["name"]
    samples = counter["samples"] || counter["sampleGroups"]&.first&.dig("samples")
    next unless samples
    counts = samples["count"] || []
    next if counts.empty?

    # Cumulative? Memory usage hook deltas need to be summed for RSS-over-time.
    if name.to_s.match?(/memory|rss/i)
      running = 0
      curve = counts.map { |d| running += d.to_i }
    else
      curve = counts.map(&:to_i)
    end
    next if curve.empty?
    min = curve.min
    max = curve.max
    range = (max - min).nonzero? || 1
    blocks = "▁▂▃▄▅▆▇█".chars
    spark = curve.map { |v| blocks[((v - min).to_f / range * (blocks.size - 1)).round] }.join
    out << "## Counter: #{name}\n\n```\n#{spark}\n```\n"
    out << "peak: #{format_bytes(max)}; min: #{format_bytes(min)}; samples: #{curve.size}\n\n"
  end
  out
end

# --- diff mode ---------------------------------------------------------------

def render_diff(baseline_path, after_path)
  base = load_profile(baseline_path)
  aft  = load_profile(after_path)
  base_main = (base["threads"] || []).find { |t| t["isMainThread"] } || (base["threads"] || []).first
  aft_main  = (aft["threads"] || []).find  { |t| t["isMainThread"] } || (aft["threads"] || []).first
  return "missing thread data" unless base_main && aft_main

  base_data = accumulate(base_main)
  aft_data  = accumulate(aft_main)
  base_resolve = resolver_for(base_main)
  aft_resolve  = resolver_for(aft_main)

  # Key by (name, file:line) so diff survives funcTable index renumbering.
  base_by_key = {}
  base_data.each do |fi, v|
    key = base_resolve.call(fi)
    base_by_key[key] = (base_by_key[key] || 0) + v[:self]
  end
  aft_by_key = {}
  aft_data.each do |fi, v|
    key = aft_resolve.call(fi)
    aft_by_key[key] = (aft_by_key[key] || 0) + v[:self]
  end

  base_dur_ms  = duration_ms_from_samples(base_main)
  aft_dur_ms   = duration_ms_from_samples(aft_main)
  base_total   = base_by_key.values.sum
  aft_total    = aft_by_key.values.sum
  to_ms = ->(w, total, dur) { total.zero? ? 0.0 : (w.to_f / total * dur) }

  keys = (base_by_key.keys + aft_by_key.keys).uniq
  rows = keys.map do |k|
    b = to_ms.call(base_by_key[k] || 0, base_total, base_dur_ms)
    a = to_ms.call(aft_by_key[k]  || 0, aft_total, aft_dur_ms)
    delta = a - b
    pct = b.zero? ? (a.zero? ? 0.0 : Float::INFINITY) : (delta / b * 100.0)
    [k, b, a, delta, pct]
  end
  rows.sort_by! { |r| -r[3].abs }
  rows = rows.first(40)
  out = String.new
  out << "# Diff: #{File.basename(baseline_path)} → #{File.basename(after_path)}\n\n"
  out << format_table(
    %w[function file:line baseline_ms after_ms delta_ms delta_%],
    rows.map { |(name, loc), b, a, d, p| [name, loc, b.round(1), a.round(1), d.round(1), (p.finite? ? p.round(1) : "new")] }
  )
  out
end

# --- CLI ---------------------------------------------------------------------

def main(argv)
  out_path = nil
  paths = []
  diff = nil
  i = 0
  while i < argv.length
    case argv[i]
    when "--out"
      out_path = argv[i + 1]
      i += 2
    when "--diff"
      diff = [argv[i + 1], argv[i + 2]]
      i += 3
    when "-h", "--help"
      puts File.read(__FILE__).lines[1..14].join.gsub(/^# ?/, "")
      return 0
    else
      paths << argv[i]
      i += 1
    end
  end

  output = String.new
  if diff
    output << render_diff(*diff)
  elsif paths.empty?
    $stderr.puts "Usage: ruby bin/analyze_profile.rb <profile.vernier.json> [--out report.md]"
    $stderr.puts "       ruby bin/analyze_profile.rb --diff baseline.json after.json"
    return 1
  else
    paths.each do |path|
      output << render_profile(path, load_profile(path))
      output << "\n"
    end
  end

  if out_path
    File.write(out_path, output)
    $stderr.puts "Wrote: #{out_path}"
  else
    print output
  end
  0
end

exit(main(ARGV)) if $PROGRAM_NAME == __FILE__
