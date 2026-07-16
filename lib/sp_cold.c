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
#include "sp_alloc.h"   /* sp_str_alloc / sp_str_set_len / sp_raise_cls */
#include "sp_array.h"   /* sp_StrArray for Dir.glob */
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include "sp_time.h"   /* sp_Time for File.mtime */
#include "sp_io.h"     /* sp_file_directory prototype */

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

/* ---- String#to_c parse + Dir.glob (cold; moved from sp_runtime.h) ---- */

sp_Complex sp_str_to_c(const char *s) {
  double re = 0, im = 0;
  int parsed = 0;
  const char *fin = s;   /* first byte NOT consumed by the parse */
  if (s) {
    const char *p = s;
    while (*p == ' ' || *p == '\t') p++;
    char *end = NULL;
    double a = strtod(p, &end);
    if (end != p) {
      parsed = 1;
      /* rational-syntax component "n/d" */
      if (*end == '/') { const char *dp = end + 1; char *de = NULL; double d = strtod(dp, &de); if (de != dp) { a /= d; end = de; } }
      if (*end == 'i') { im = a; fin = end + 1; }
      else {
        re = a;
        const char *q = end;
        double b2 = strtod(q, &end);
        if (end != q) {
          if (*end == '/') { const char *dp = end + 1; char *de = NULL; double d = strtod(dp, &de); if (de != dp) { b2 /= d; end = de; } }
          if (*end == 'i') { im = b2; fin = end + 1; }
          else fin = q;   /* an imaginary number without the 'i' suffix ("1+2") is invalid */
        }
        else if ((*q == '+' || *q == '-') && q[1] == 'i') { im = (*q == '-') ? -1.0 : 1.0; fin = q + 2; }
        else fin = q;        /* "1+" and other incomplete forms leave the operator unconsumed */
      }
    }
    else if (*p == 'i') { im = 1; parsed = 1; fin = p + 1; }
    else if ((*p == '+' || *p == '-') && p[1] == 'i') { im = (*p == '-') ? -1.0 : 1.0; parsed = 1; fin = p + 2; }
  }
  /* the whole string must be consumed (only trailing whitespace allowed): an
     incomplete form like "1+" is invalid, not silently (1+0i) (#2617). */
  if (parsed) { while (*fin == ' ' || *fin == '\t') fin++; if (*fin != '\0') parsed = 0; }
  /* an unparseable string (or nil) is not a valid Complex value */
  if (!parsed) sp_raise_cls("ArgumentError", "invalid value for convert(): ");
  return (sp_Complex){ (mrb_float)re, (mrb_float)im };
}

int sp_fnmatch1(const char *pat, const char *str) {
  while (*pat) {
    if (*pat == '*') {
      pat++;
      if (!*pat) return 1;
      while (*str) { if (sp_fnmatch1(pat, str)) return 1; str++; }
      return sp_fnmatch1(pat, str);
    }
else if (*pat == '?') {
      if (!*str) return 0;
      pat++; str++;
    }
else {
      if (*pat != *str) return 0;
      pat++; str++;
    }
  }
  return *str == 0;
}

void sp_dir_glob_rec(const char *fsdir, const char *outprefix,
                            const char *tail, sp_StrArray *a) {
  DIR *d = opendir(fsdir);
  if (!d) return;
  struct dirent *e;
  while ((e = readdir(d)) != NULL) {
    const char *name = e->d_name;
    if (name[0] == '.' && (name[1] == 0 || (name[1] == '.' && name[2] == 0))) continue;
    char fspath[2048], outpath[2048];
    snprintf(fspath, sizeof fspath, "%s/%s", fsdir, name);
    snprintf(outpath, sizeof outpath, "%s%s", outprefix, name);
    if (!(name[0] == '.' && tail[0] != '.') && sp_fnmatch1(tail, name)) {
      char *copy = sp_str_alloc(strlen(outpath));
      strcpy(copy, outpath);
      sp_StrArray_push(a, copy);
    }
    if (name[0] != '.') {
      struct stat st;
      if (lstat(fspath, &st) == 0 && S_ISDIR(st.st_mode)) {
        char subprefix[2048];
        snprintf(subprefix, sizeof subprefix, "%s%s/", outprefix, name);
        sp_dir_glob_rec(fspath, subprefix, tail, a);
      }
    }
  }
  closedir(d);
}

sp_StrArray *sp_dir_glob(const char *pattern) {
  /* the pattern is often a fresh interpolation temp, unrooted at the call
     site; the per-match sp_str_allocs below can collect it mid-walk */
  SP_GC_ROOT_STR(pattern);
  sp_StrArray *a = sp_StrArray_new();
  /* every matched entry sp_str_allocs inside the walk below, and enough of
     them trigger a collection mid-build -- root the result like
     sp_dir_entries_impl or it (and its pushed names) get swept under us */
  SP_GC_ROOT(a);
  if (!pattern) return a;
  /* Recursive double-star form: split at the double-star component. Everything
     before it is the output prefix (and, minus the trailing slash, the directory
     to walk); the component after it is the per-directory tail pattern. */
  const char *ss = strstr(pattern, "**");
  if (ss) {
    size_t plen = (size_t)(ss - pattern);
    if (plen >= 1024) return a;
    const char *after = ss + 2;
    if (*after == '/') after++;
    const char *tail = after;
    char outprefix[1024];
    memcpy(outprefix, pattern, plen);
    outprefix[plen] = 0;
    char fsdir[1024];
    if (plen == 0) {
      strcpy(fsdir, ".");
    }
    else {
      memcpy(fsdir, pattern, plen);
      fsdir[plen] = 0;
      if (fsdir[plen - 1] == '/') fsdir[plen - 1] = 0;
      if (fsdir[0] == 0) strcpy(fsdir, "/");
    }
    sp_dir_glob_rec(fsdir, outprefix, tail, a);
    sp_StrArray_sort_bang(a);
    return a;
  }
  const char *slash = strrchr(pattern, '/');
  char dirbuf[1024];
  const char *dirpath;
  const char *base_pat;
  if (slash) {
    size_t dl = (size_t)(slash - pattern);
    if (dl >= sizeof(dirbuf)) return a;
    memcpy(dirbuf, pattern, dl);
    dirbuf[dl] = 0;
    dirpath = (dl == 0) ? "/" : dirbuf;
    base_pat = slash + 1;
  }
else {
    dirpath = ".";
    base_pat = pattern;
  }
  DIR *d = opendir(dirpath);
  if (!d) return a;
  struct dirent *e;
  while ((e = readdir(d)) != NULL) {
    const char *name = e->d_name;
    if (name[0] == '.' && base_pat[0] != '.') continue;
    if (sp_fnmatch1(base_pat, name)) {
      char full[2048];
      if (slash) snprintf(full, sizeof(full), "%s/%s", dirbuf, name);
      else snprintf(full, sizeof(full), "%s", name);
      char *copy = sp_str_alloc(strlen(full));
      strcpy(copy, full);
      sp_StrArray_push(a, copy);
    }
  }
  closedir(d);
  sp_StrArray_sort_bang(a);
  return a;
}

/* ---- File.read/size/mtime/join/readlines + Math.lgamma (cold) ---- */

const char *sp_file_join(const char **parts, int n) {
  size_t total = 0;
  for (int i = 0; i < n; i++) {
    if (parts[i]) total += strlen(parts[i]);
    if (i < n - 1) total++;
  }
  char *r = sp_str_alloc((mrb_int)total);
  size_t off = 0;
  for (int i = 0; i < n; i++) {
    if (parts[i]) { size_t l = strlen(parts[i]); memcpy(r + off, parts[i], l); off += l; }
    if (i < n - 1) r[off++] = '/';
  }
  r[off] = 0;
  return r;
}

sp_StrArray *sp_file_readlines(const char *path) {
  sp_StrArray *a = sp_StrArray_new();
  SP_GC_ROOT(a);
  FILE *_fp = fopen(path ? path : "", "r");
  if (!_fp) return a;
  char _buf[4096];
  while (fgets(_buf, (int)sizeof(_buf), _fp)) {
    size_t _l = strlen(_buf);
    char *_r = sp_str_alloc_raw(_l + 1);
    memcpy(_r, _buf, _l + 1);
    sp_StrArray_push(a, _r);
  }
  fclose(_fp);
  return a;
}

sp_StrArray *sp_file_readlines_chomp(const char *path) {
  sp_StrArray *a = sp_StrArray_new();
  SP_GC_ROOT(a);
  FILE *_fp = fopen(path ? path : "", "r");
  if (!_fp) return a;
  char _buf[4096];
  while (fgets(_buf, (int)sizeof(_buf), _fp)) {
    size_t _l = strlen(_buf);
    if (_l > 0 && _buf[_l-1] == '\n') { _buf[--_l] = '\0'; }
    if (_l > 0 && _buf[_l-1] == '\r') { _buf[--_l] = '\0'; }
    char *_r = sp_str_alloc_raw(_l + 1);
    memcpy(_r, _buf, _l + 1);
    sp_StrArray_push(a, _r);
  }
  fclose(_fp);
  return a;
}

double sp_lgamma_pos(double x) {  /* x > 0 */
  if (x == 1.0 || x == 2.0) return 0.0;
  double corr = 0.0;
  while (x < 12.0) { corr -= log(x); x += 1.0; }
  double inv = 1.0 / x, inv2 = inv * inv;
  /* sum_{k>=1} B_2k / (2k(2k-1) x^(2k-1)) up to the 1/x^11 term */
  double series = (1.0/12.0) + (inv2 * (-(1.0/360.0) + (inv2 * ((1.0/1260.0)
                  + (inv2 * (-(1.0/1680.0) + (inv2 * (1.0/1188.0))))))));
  return corr + ((x - 0.5) * log(x)) - x + (0.5 * log(2.0 * M_PI)) + (series * inv);
}

sp_PolyArray *sp_math_lgamma(double x) {
  int sign = 1; double v;
  if (x > 0.0) {
    v = sp_lgamma_pos(x);
  }
  else {
    double s = sin(M_PI * x);
    if (s == 0.0) v = INFINITY;            /* pole at a non-positive integer */
    else { if (s < 0.0) sign = -1; v = log(M_PI / fabs(s)) - sp_lgamma_pos(1.0 - x); }
  }
  sp_PolyArray *r = sp_PolyArray_new(); SP_GC_ROOT(r);
  sp_PolyArray_push(r, sp_box_float(v));
  sp_PolyArray_push(r, sp_box_int(sign));
  return r;
}

const char *sp_file_read(const char *path) {
  if (sp_file_directory(path)) {
    sp_raise_cls("Errno::EISDIR", sp_sprintf("Is a directory @ io_fread - %s", path));
  }
  FILE *f = fopen(path, "r");
  if (!f) {
    sp_raise_cls(errno == ENOENT ? "Errno::ENOENT" : errno == EACCES ? "Errno::EACCES" : "RuntimeError",
                 sp_sprintf("%s @ rb_sysopen - %s", strerror(errno), path));
    return &("\xff" "")[1];
  }
  fseek(f, 0, SEEK_END);
  long sz = ftell(f);
  fseek(f, 0, SEEK_SET);
  char *buf = sp_str_alloc(sz);
  size_t n = 0;
  if (sz > 0) {
    n = fread(buf, 1, sz, f);
  }
  buf[n] = 0;
  sp_str_set_len(buf, n);  /* a short read must not leave the size as length */
  fclose(f);
  return buf;
}

sp_Time sp_file_mtime(const char *path) {
  if (!path) {
    sp_raise_cls("TypeError", "no implicit conversion of nil into String");
    return (sp_Time){0, 0, 0};
  }
  struct stat st;
  if (stat(path, &st) == -1) {
    sp_raise_cls(errno == ENOENT ? "Errno::ENOENT" : "RuntimeError", sp_sprintf("%s @ File.mtime - %s", strerror(errno), path));
    return (sp_Time){0, 0, 0};
  }
#if defined(__APPLE__)
  return (sp_Time){(int64_t)st.st_mtimespec.tv_sec, (int32_t)st.st_mtimespec.tv_nsec, 0};
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__DragonFly__)
  return (sp_Time){(int64_t)st.st_mtimespec.tv_sec, (int32_t)st.st_mtimespec.tv_nsec, 0};
#else
  /* Linux / others with st_mtim */
  return (sp_Time){(int64_t)st.st_mtim.tv_sec, (int32_t)st.st_mtim.tv_nsec, 0};
#endif
}

mrb_int sp_file_size(const char *path) {
  if (!path) {
    sp_raise_cls("TypeError", "no implicit conversion of nil into String");
    return 0;
  }
  struct stat st;
  if (stat(path, &st) == -1) {
    int err = errno;  /* capture once: strerror() may clobber errno */
    sp_raise_cls(err == ENOENT ? "Errno::ENOENT" : "RuntimeError", sp_sprintf("%s @ File.size - %s", strerror(err), path));
    return 0;
  }
  /* off_t (typically 64-bit) into mrb_int (intptr_t -> 32-bit on a 32-bit
     build): guard the narrowing, as spinel does for int arithmetic. */
  if ((off_t)(mrb_int)st.st_size != st.st_size) {
    sp_raise_cls("RangeError", "file size out of range for Integer");
    return 0;
  }
  return (mrb_int)st.st_size;
}

sp_IntArray *sp_file_binread_bytes(const char *path) {
  if (sp_file_directory(path)) {
    sp_raise_cls("Errno::EISDIR", sp_sprintf("Is a directory @ io_fread - %s", path));
  }
  FILE *f = fopen(path, "rb");
  sp_IntArray *a = sp_IntArray_new();
  if (!f) {
    sp_raise_cls(errno == ENOENT ? "Errno::ENOENT" : errno == EACCES ? "Errno::EACCES" : "RuntimeError",
                 sp_sprintf("%s @ rb_sysopen - %s", strerror(errno), path));
    return a;
  }
  fseek(f, 0, SEEK_END);
  long sz = ftell(f);
  fseek(f, 0, SEEK_SET);
  unsigned char *buf = (unsigned char *)malloc(sz > 0 ? (size_t)sz : 1);
  if (buf && sz > 0) {
    /* Use fread's actual byte count, not the raw file size — a
       partial read otherwise pushes uninitialized memory. */
    size_t r = fread(buf, 1, (size_t)sz, f);
    for (size_t i = 0; i < r; i++) sp_IntArray_push(a, (mrb_int)buf[i]);
  }
  free(buf);
  fclose(f);
  return a;
}
