# Per-file frozen_string_literal under the fsl-true DEFAULT: a file with no
# pragma is frozen (the default), an explicit `false` opts a file back into
# plain mutable literals, and each literal carries its OWN file's setting.
# helper_frozen.rb has the true pragma; helper_plain.rb has an explicit
# false; this entry file has no pragma (-> default frozen).
require_relative "frozen_string_literal_per_file/helper_frozen"
require_relative "frozen_string_literal_per_file/helper_plain"

p frozen_helper_lit.frozen?   # true  -- literal from the pragma file
p plain_helper_lit.frozen?    # false -- explicit false file
p "entry lit".frozen?         # true  -- no pragma: the fsl default

# mutating a pragma-file literal raises even when called from a plain file
begin
  frozen_helper_mutate
  puts "BUG: no raise"
rescue FrozenError => e
  puts e.message
end

# an explicit-false file's literal stays mutable
p plain_helper_build
