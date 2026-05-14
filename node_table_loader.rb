# Loads Spinel's text AST format into a node table object.
#
# The table object supplies the parallel-array storage operations used by
# Compiler: alloc_node, set_root_id, set_node_type, set_node_content, and
# set_*_field.

class NodeTableLoader
  def initialize(table)
    @table = table
  end

  def read_text_ast(data)
    lines = data.split(10.chr)
 # Pass 1: find max node ID
    max_id = 0
    i = 0
    while i < lines.length
      line = lines[i]
      if line.length > 0
        parts = line.split(" ")
        if parts.length >= 2
          if parts.first == "ROOT"
            @table.set_root_id(parts[1].to_i)
          end
          if parts.first == "N"
            nid = parts[1].to_i
            if nid > max_id
              max_id = nid
            end
          end
        end
      end
      i = i + 1
    end
 # Allocate nodes
    j = 0
    while j <= max_id
      @table.alloc_node
      j = j + 1
    end
 # Pass 2: populate fields
    i = 0
    while i < lines.length
      line = lines[i]
      if line.length > 0
        ast_parse_line(line)
      end
      i = i + 1
    end
  end

  def ast_parse_line(line)
    parts = line.split(" ")
    if parts.length < 3
      return
    end
    tag = parts.first
    nid = parts[1].to_i
    if tag == "N"
      @table.set_node_type(nid, parts[2])
    elsif tag == "S"
      field = parts[2]
      val = ""
      if parts.length >= 4
        val = unescape_str(parts[3])
      end
      @table.set_string_field(nid, field, val)
    elsif tag == "I"
      field = parts[2]
      ival = 0
      if parts.length >= 4
        ival = parts[3].to_i
      end
      @table.set_int_field(nid, field, ival)
    elsif tag == "F"
      if parts.length >= 4
        @table.set_node_content(nid, parts[3])
      end
    elsif tag == "R"
      field = parts[2]
      ref_id = -1
      if parts.length >= 4
        ref_id = parts[3].to_i
      end
      @table.set_ref_field(nid, field, ref_id)
    elsif tag == "A"
      field = parts[2]
      ids_str = ""
      if parts.length >= 4
        ids_str = parts[3]
      end
      @table.set_array_field(nid, field, ids_str)
    end
    0
  end

  def unescape_str(s)
    result = ""
    i = 0
    n = s.length
    while i < n
      ch = s[i]
      if ch == "%" && i + 2 < n
        hi = hex_digit(s[i + 1])
        lo = hex_digit(s[i + 2])
        if hi >= 0 && lo >= 0
          result = result + (hi * 16 + lo).chr
          i = i + 3
        else
          result = result + ch
          i = i + 1
        end
      else
        result = result + ch
        i = i + 1
      end
    end
    result
  end

 # Returns 0..15 for a hex digit char ("0".."9","A".."F","a".."f"), -1
 # otherwise. Used by unescape_str to decode arbitrary %HH bytes — high
 # bytes (>= 0x80) from string literals like the PNG magic must round-
 # trip; the previous case-list approach passed them through as literal
 # "%89", corrupting binary string contents in the AST.
  def hex_digit(ch)
    code = ch.bytes[0]
    if code >= 48 && code <= 57
      code - 48
    elsif code >= 65 && code <= 70
      code - 55
    elsif code >= 97 && code <= 102
      code - 87
    else
      -1
    end
  end
end
