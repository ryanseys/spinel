# Stress-test String#split with the four-level delimiter pattern used
# by spinel's text-IR loader: outer newlines wrap per-record lines;
# each line is space-delimited into fields; some fields carry
# pipe-separated groups; each group holds semicolon-separated pairs;
# each pair holds comma-separated values. This mirrors what
# `node_table_loader.rb` and `spinel_analyze.rb` do on every IR load
# -- per the vernier self-compile profile, ~40% of wall time is
# inside String#split.
#
# Output is a deterministic integer (total field count summed across
# all inner splits) so the bench harness can correctness-check spinel
# vs MRI. Wall-time measurement is external (`/usr/bin/time` around
# the compiled binary or the harness timing).

# Build a fake IR-shaped corpus: 500 lines with the multi-level
# delimiter shape.
input = ""
i = 0
while i < 500
  input = input + "T " + i.to_s + " obj_Cls" + (i % 38).to_s + " field|name1,val1,extra;name2,val2;name3,val3|other;a,b,c\n"
  i = i + 1
end

# Repeat the parse 200 times so the work dominates startup overhead.
total = 0
iter = 0
while iter < 200
  lines = input.split("\n")
  j = 0
  while j < lines.length
    parts = lines[j].split(" ")
    k = 0
    while k < parts.length
      groups = parts[k].split("|")
      m = 0
      while m < groups.length
        pairs = groups[m].split(";")
        p = 0
        while p < pairs.length
          fields = pairs[p].split(",")
          total = total + fields.length
          p = p + 1
        end
        m = m + 1
      end
      k = k + 1
    end
    j = j + 1
  end
  iter = iter + 1
end

puts total
