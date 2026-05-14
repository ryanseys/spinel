# Shared between spinel_analyze.rb and spinel_codegen.rb. Classifies
# the literal string passed to Integer("...") and emits the matching
# sp_bigint_new_str call for the literal-Bignum codegen path.
#
# Two entry points, each with a single typed return value (string),
# because spinel's static type inference doesn't yet support
# destructuring a mixed-type array return cleanly. The internal
# prefix-detection loop is intentionally duplicated between the two
# methods — that duplication lives inside this file, not across the
# analyze/codegen boundary.
#
# Pure: no instance state, no raise/rescue (avoids bootstrap
# complications inside the self-hosted analyzer). Anything
# unparseable returns "int" so the runtime parser takes over and
# raises ArgumentError uniformly.

class IntegerLiteral
 # Returns "int" or "bigint" for the magnitude of `lit`. Conservative:
 # an unrecognized prefix or empty digit body returns "int". Digit
 # count thresholds per base:
 #   decimal: 20+ → bigint; 19 compared against LLONG_MAX / MIN_ABS
 #   hex:     17+ → bigint; 16 compared against 0x7F.../0x80...
 #   octal:   22+ → bigint
 #   binary:  64+ → bigint
  def self.classify(lit)
    return "int" if lit.nil? || lit.empty?
    s = lit.strip
    return "int" if s.empty?
    negative = false
    if s.start_with?("-")
      negative = true
      s = s[1, s.length - 1]
      return "int" if s.nil? || s.empty?
    elsif s.start_with?("+")
      s = s[1, s.length - 1]
      return "int" if s.nil? || s.empty?
    end
    base = 10
    digits = s
    if s.start_with?("0x") || s.start_with?("0X")
      base = 16; digits = s[2, s.length - 2]
    elsif s.start_with?("0b") || s.start_with?("0B")
      base = 2; digits = s[2, s.length - 2]
    elsif s.start_with?("0o") || s.start_with?("0O")
      base = 8; digits = s[2, s.length - 2]
    elsif s.start_with?("0d") || s.start_with?("0D")
      base = 10; digits = s[2, s.length - 2]
    elsif s.length >= 2 && s.start_with?("0") && s[1] >= "0" && s[1] <= "9"
      base = 8; digits = s[1, s.length - 1]
    end
    return "int" if digits.nil? || digits.empty?
    raw = digits.gsub("_", "")
    return "int" if raw.empty?
    if base == 10
      return "bigint" if raw.length >= 20
      if raw.length == 19
        limit = negative ? "9223372036854775808" : "9223372036854775807"
        return "bigint" if raw > limit
      end
    elsif base == 16
      return "bigint" if raw.length >= 17
      if raw.length == 16
        limit = negative ? "8000000000000000" : "7FFFFFFFFFFFFFFF"
        return "bigint" if raw.upcase > limit
      end
    elsif base == 8
      return "bigint" if raw.length >= 22
    elsif base == 2
      return "bigint" if raw.length >= 64
    end
    "int"
  end

 # For a literal that .classify already returned "bigint" for, emit
 # the matching `sp_bigint_new_str("<body>", <base>)` C call. mini-gmp's
 # mpz_init_set_str handles underscores and a leading sign at every
 # base but does NOT understand the 0x/0b/0o/0d prefix — strip it
 # here. Legacy octal ("0" + digits) keeps its leading zero because
 # base 8 reads it as a no-op digit. Integer-literal strings contain
 # only [0-9A-Za-z+\-_], so wrapping the body in C "..." needs no
 # escaping.
  def self.bigint_call(lit)
    s = lit.strip
    sign = ""
    if s.start_with?("-") || s.start_with?("+")
      sign = s[0, 1]
      s = s[1, s.length - 1]
    end
    base = 10
    body = s
    if s.start_with?("0x") || s.start_with?("0X")
      base = 16; body = s[2, s.length - 2]
    elsif s.start_with?("0b") || s.start_with?("0B")
      base = 2; body = s[2, s.length - 2]
    elsif s.start_with?("0o") || s.start_with?("0O")
      base = 8; body = s[2, s.length - 2]
    elsif s.start_with?("0d") || s.start_with?("0D")
      base = 10; body = s[2, s.length - 2]
    elsif s.length >= 2 && s.start_with?("0") && s[1] >= "0" && s[1] <= "9"
      base = 8
    end
    "sp_bigint_new_str(\"" + sign + body + "\", " + base.to_s + ")"
  end
end
