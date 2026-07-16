/* Cold, out-of-line helpers extracted from sp_runtime.h.
 *
 * Functions that are large and not on any hot path were `static` in
 * sp_runtime.h, so every translation unit that includes the header (every
 * generated program, plus each lib TU) compiled its own copy. Moving them here
 * -- a single TU that includes the full runtime header -- gives one linked copy
 * and shrinks per-TU compile work. sp_runtime.h keeps an `extern` declaration
 * so all callers still resolve. Only functions that call no program-generated
 * static (sp_sym_to_s / sp_sym_intern / ...) can live here, and each includes
 * only the low-level headers its dependencies need -- NOT sp_runtime.h, which
 * is a monolithic definition header meant to be compiled once (into the
 * generated program), so including it here would multiply-define the runtime. */
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "sp_alloc.h"   /* sp_str_alloc / sp_str_set_len */

/* Integer#% / Kernel#format "%b"/"%B": binary formatting with Ruby's flag,
   width, precision, and two's-complement-for-negative rules. */
int sp_fmt_binary(const char *spec, size_t sl, char conv, long long val,
                  char *out, size_t osz) {
  /* parse "%<flags><width>.<prec>b" out of spec[0..sl-1] (spec[sl-1] == conv) */
  int f_minus = 0, f_plus = 0, f_space = 0, f_hash = 0, f_zero = 0;
  size_t i = 1;
  for (; i < sl; i++) {
    if (spec[i] == '-') f_minus = 1;
    else if (spec[i] == '+') f_plus = 1;
    else if (spec[i] == ' ') f_space = 1;
    else if (spec[i] == '#') f_hash = 1;
    else if (spec[i] == '0') f_zero = 1;
    else break;
  }
  int width = 0;
  for (; i < sl && spec[i] >= '0' && spec[i] <= '9'; i++) width = (width * 10) + (spec[i] - '0');
  int prec = -1;
  if (i < sl && spec[i] == '.') {
    i++; prec = 0;
    for (; i < sl && spec[i] >= '0' && spec[i] <= '9'; i++) prec = (prec * 10) + (spec[i] - '0');
  }

  int neg = val < 0;
  /* Ruby shows the two's-complement ".." body only for a negative value with no
     sign flag; a + or space flag switches to signed magnitude ("-101"). */
  int twos = neg && !f_plus && !f_space;
  char digits[256]; int dn = 0;
  if (twos) {
    unsigned long long uv = (unsigned long long)val;
    int p = -1;  /* highest 0-bit position; -1 means val == -1 (all ones) */
    for (int bit = 62; bit >= 0; bit--) if (!((uv >> bit) & 1ULL)) { p = bit; break; }
    int ndig = p + 2; if (ndig < 1) ndig = 1;
    for (int bit = ndig - 1; bit >= 0; bit--) digits[dn++] = (char)('0' + (int)((uv >> bit) & 1ULL));
  } else {
    /* signed magnitude: |val| in binary. 0 has no significant digits, so it
       contributes a single '0' only when precision is not 0. */
    unsigned long long mag = neg ? (unsigned long long)(-(val + 1)) + 1 : (unsigned long long)val;
    if (mag == 0) { if (prec != 0) digits[dn++] = '0'; }
    else { char t[80]; int tn = 0; while (mag) { t[tn++] = (char)('0' + (int)(mag & 1ULL)); mag >>= 1; }
           while (tn) digits[dn++] = t[--tn]; }
  }
  /* precision: minimum digit count. The ".." body counts as 2 toward it and pads
     with the sign bit (1); signed magnitude pads with 0. */
  if (prec >= 0) {
    int target = twos ? (prec - 2) : prec;
    char padc = twos ? '1' : '0';
    int t2 = target - dn;
    if (t2 > 0) {
      /* clamp to the digits buffer (output is capped at osz anyway) */
      if (t2 + dn >= (int)sizeof(digits)) t2 = (int)sizeof(digits) - dn - 1;
      memmove(digits + t2, digits, (size_t)dn);  /* shift body right */
      memset(digits, padc, (size_t)t2);           /* leading padding */
      dn += t2;
    }
    f_zero = 0;  /* precision disables 0-flag for integer conversions */
  }
  /* assemble sign/prefix + body, then apply width padding */
  char body[300]; int bn = 0;
  char sign = twos ? 0 : (neg ? '-' : (f_plus ? '+' : (f_space ? ' ' : 0)));
  char prefix0 = 0, prefix1 = 0;
  if (f_hash && val != 0) { prefix0 = '0'; prefix1 = (conv == 'B') ? 'B' : 'b'; }
  if (sign) body[bn++] = sign;
  if (prefix0) { body[bn++] = prefix0; body[bn++] = prefix1; }
  if (twos) { body[bn++] = '.'; body[bn++] = '.'; }
  for (int k = 0; k < dn; k++) body[bn++] = digits[k];

  int o = 0;
  int pad = width - bn;
  if (pad > 0 && !f_minus && f_zero) {
    /* zero-pad: emit sign/prefix/".." first, then fill, then the rest. A two's-
       complement body fills with the sign bit (1); signed magnitude with 0. */
    int head = (sign ? 1 : 0) + (prefix0 ? 2 : 0) + (twos ? 2 : 0);
    char fillc = twos ? '1' : '0';
    for (int k = 0; k < head && o < (int)osz; k++) out[o++] = body[k];
    for (int k = 0; k < pad && o < (int)osz; k++) out[o++] = fillc;
    for (int k = head; k < bn && o < (int)osz; k++) out[o++] = body[k];
  } else {
    if (pad > 0 && !f_minus) for (int k = 0; k < pad && o < (int)osz; k++) out[o++] = ' ';
    for (int k = 0; k < bn && o < (int)osz; k++) out[o++] = body[k];
    if (pad > 0 && f_minus) for (int k = 0; k < pad && o < (int)osz; k++) out[o++] = ' ';
  }
  return o;
}

/* File.expand_path(path[, base]): absolute, `.`/`..`/`//`-normalized path.
   Depends only on sp_alloc.h + libc; no program-generated symbols. */
const char *sp_file_expand_path(const char *path, const char *base) {
  char raw[8192];
  char cwd[4096];
  const char *home = getenv("HOME");
  if (!home) home = "";
  if (!path) path = "";

  /* The `%.4000s` precision caps each component so the compiler can
     prove the combined output (<= 8001) fits in the 8192 buffer -- this
     silences -Wformat-truncation (an error under the test harness's
     -Werror). 4000 chars/component is well past PATH_MAX, so real paths
     never truncate. */
  if (path[0] == '~' && (path[1] == '\0' || path[1] == '/')) {
    snprintf(raw, sizeof(raw), "%.4000s%.4000s", home, path + 1);
  }
  else if (path[0] == '/') {
    snprintf(raw, sizeof(raw), "%.4000s", path);
  }
  else {
    char basebuf[8192];
    const char *b;
    if (base && base[0]) {
      if (base[0] == '~' && (base[1] == '\0' || base[1] == '/')) {
        snprintf(basebuf, sizeof(basebuf), "%.4000s%.4000s", home, base + 1);
        b = basebuf;
      }
      else if (base[0] == '/') {
        b = base;
      }
      else {
        if (!getcwd(cwd, sizeof(cwd))) cwd[0] = 0;
        snprintf(basebuf, sizeof(basebuf), "%.4000s/%.4000s", cwd, base);
        b = basebuf;
      }
    }
    else {
      if (!getcwd(cwd, sizeof(cwd))) cwd[0] = 0;
      b = cwd;
    }
    snprintf(raw, sizeof(raw), "%.4000s/%.4000s", b, path);
  }

  /* Normalize: walk segments, collapsing `.`/`..`/`//`. seg_start[k]
     records the output length to roll back to when a `..` pops the
     k-th kept segment. */
  size_t rawlen = strlen(raw);
  char *out = sp_str_alloc(rawlen + 1);
  size_t seg_start[1024];
  int nseg = 0;
  size_t olen = 0;
  out[olen++] = '/';
  const char *p = raw;
  while (*p) {
    if (*p == '/') { p++; continue; }
    const char *q = p;
    while (*q && *q != '/') q++;
    size_t slen = (size_t)(q - p);
    if (slen == 1 && p[0] == '.') {
      /* current dir -- skip */
    }
    else if (slen == 2 && p[0] == '.' && p[1] == '.') {
      if (nseg > 0) { nseg--; olen = seg_start[nseg]; }
    }
    else {
      size_t mark = olen;
      if (olen > 1) out[olen++] = '/';
      memcpy(out + olen, p, slen);
      olen += slen;
      if (nseg < 1024) seg_start[nseg++] = mark;
    }
    p = q;
  }
  out[olen] = 0;
  sp_str_set_len(out, olen);
  return out;
}
