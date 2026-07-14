/* sp_format.c -- cold value-type display helpers (see sp_format.h).
   Self-contained: the shared value types (sp_types.h) + string allocator
   (sp_alloc.h) + libc formatting only. */
#include "sp_format.h"
#include "sp_alloc.h"   /* sp_str_alloc_raw, sp_raise_cls, <math.h> for cos/sin/sqrt */
#include <stdio.h>
#include <string.h>
#include <time.h>       /* gmtime / strftime for sp_Time_inspect */

/* Format a non-negative Complex-component magnitude the way MRI does: infinite
   and NaN values become the Ruby names Infinity/NaN (not C's inf/nan), a
   Float-classed component (is_f) renders Ruby-float style ("2.0"), a whole
   Integer-classed value stays integer-looking, else %g. Returns the length. */
static int sp_complex_mag(char *out, size_t sz, mrb_float v, int is_f) {
  if (isinf(v)) return snprintf(out, sz, "Infinity");
  if (isnan(v)) return snprintf(out, sz, "NaN");
  if (is_f) return snprintf(out, sz, "%s", sp_float_to_s(v));
  /* a float outside mrb_int's range makes (mrb_int)v undefined behavior; only
     integer-print a whole value that fits, else fall through to %g. */
  if (v >= -(mrb_float)INTPTR_MAX && v <= (mrb_float)INTPTR_MAX && v == (mrb_int)v)
    return snprintf(out, sz, "%lld", (long long)v);
  return snprintf(out, sz, "%g", v);
}
/* Render one component's magnitude (absolute value) of `c` into out. `im`
   selects the imaginary component; `insp` wraps an exact Rational in parens
   (inspect form `(1/2)`) vs bare (to_s form `1/2`). Returns the component's
   sign (negative -> -1, else 0). An exact Rational prints as its reduced n/d;
   a Float/Integer part defers to sp_complex_mag's float-or-whole rendering.
   The inspect parens on a Rational make its magnitude start with `(`, which
   the caller's `*`-separator rule then renders as `(1/1)*i` -- exactly MRI's
   split between inspect (`*`) and to_s (`1/1i`). */
static int sp_cpart_mag(char *out, size_t sz, sp_Complex c, int im, int insp) {
  if (c.exact & (im ? SP_CPLX_IM_X : SP_CPLX_RE_X)) {
    sp_Rational r = im ? c.im_r : c.re_r;
    int neg = r.num < 0;
    /* Imaginary part: the sign moves to the join (`3-(1/2)*i`), so render the
       absolute magnitude and report the sign. Real part: the sign stays with
       the value inside the parens (`(-1/2)+3i`), so render it signed and
       report a non-negative sign the caller won't re-prepend. */
    long long n = (long long)(im && neg ? -r.num : r.num);
    snprintf(out, sz, insp ? "(%lld/%lld)" : "%lld/%lld", n, (long long)r.den);
    return im && neg ? -1 : 0;
  }
  mrb_float v = im ? c.im : c.re;
  sp_complex_mag(out, sz, v < 0 ? -v : v, c.fl & (im ? SP_CPLX_IM_F : SP_CPLX_RE_F));
  return v < 0 ? -1 : 0;
}
/* MRI inserts a `*` before a non-numeric imaginary magnitude (Infinity/NaN,
   or an inspect-form Rational `(1/1)`) so it stays readable; a plain number is
   written `10i`. */
static const char *sp_cpart_imag_sep(const char *mag) {
  return (mag[0] >= '0' && mag[0] <= '9') ? "" : "*";
}
const char *sp_complex_inspect(sp_Complex c) {
  char re[80], im[80], buf[192];
  int rs = sp_cpart_mag(re, sizeof re, c, 0, 1);
  int is = sp_cpart_mag(im, sizeof im, c, 1, 1);
  int n = snprintf(buf, sizeof buf, "(%s%s%c%s%si)",
                   rs < 0 ? "-" : "", re, is < 0 ? '-' : '+', im, sp_cpart_imag_sep(im));
  if (n < 0) n = 0; else if (n >= (int)sizeof buf) n = (int)sizeof buf - 1;
  char *r = sp_str_alloc_raw(n + 1);
  memcpy(r, buf, n);
  r[n] = 0;
  return r;
}
/* Complex#to_s: bare `re+imi` (no surrounding parens, unlike #inspect). */
const char *sp_complex_to_s(sp_Complex c) {
  char re[80], im[80], buf[192];
  int rs = sp_cpart_mag(re, sizeof re, c, 0, 0);
  int is = sp_cpart_mag(im, sizeof im, c, 1, 0);
  int n = snprintf(buf, sizeof buf, "%s%s%c%s%si",
                   rs < 0 ? "-" : "", re, is < 0 ? '-' : '+', im, sp_cpart_imag_sep(im));
  if (n < 0) n = 0; else if (n >= (int)sizeof buf) n = (int)sizeof buf - 1;
  char *r = sp_str_alloc_raw(n + 1);
  memcpy(r, buf, n);
  r[n] = 0;
  return r;
}

const char *sp_rational_inspect(sp_Rational r) {
  char buf[64];
  int n = snprintf(buf, sizeof(buf), "(%lld/%lld)", (long long)r.num, (long long)r.den);
  if (n < 0) n = 0;
  char *o = sp_str_alloc_raw(n + 1);
  memcpy(o, buf, n);
  o[n] = 0;
  return o;
}
/* Rational#to_s: bare `num/den` (no parens, unlike #inspect). */
const char *sp_rational_to_s(sp_Rational r) {
  char buf[64];
  int n = snprintf(buf, sizeof(buf), "%lld/%lld", (long long)r.num, (long long)r.den);
  if (n < 0) n = 0;
  char *o = sp_str_alloc_raw(n + 1);
  memcpy(o, buf, n);
  o[n] = 0;
  return o;
}

const char *sp_Range_inspect(sp_Range *r) {
  /* "first..last" / "first...last" form. Buffer sized for two int64s + dots. */
  char *buf = sp_str_alloc_raw(48);
  snprintf(buf, 48, r->excl ? "%lld...%lld" : "%lld..%lld", (long long)r->first, (long long)r->last);
  return buf;
}

/* "YYYY-MM-DD HH:MM:SS UTC" via gmtime: the spinel runtime keeps Time
   timezone-naive, so UTC is the unambiguous choice that needs no tzdata. */
static const char *sp_Time_fmt(sp_Time *t, int frac) {
  char *buf = sp_str_alloc_raw(48);
  time_t sec = (time_t)t->tv_sec;
  struct tm *tm_ = gmtime(&sec);
  if (tm_) {
    size_t n = strftime(buf, 48, "%Y-%m-%d %H:%M:%S", tm_);
    if (frac && t->tv_nsec != 0) {
      /* fractional seconds, trailing zeros trimmed (matches sp_time_inspect_v) */
      n += (size_t)snprintf(buf + n, 48 - n, ".%09d", (int)t->tv_nsec);
      while (buf[n - 1] == '0') buf[--n] = 0;
    }
    snprintf(buf + n, 48 - n, " UTC");
  }
  else {
    snprintf(buf, 48, "Time(%lld)", (long long)t->tv_sec);
  }
  return buf;
}
/* Time#inspect renders fractional seconds; Time#to_s does not. */
const char *sp_Time_inspect(sp_Time *t) { return sp_Time_fmt(t, 1); }
const char *sp_Time_to_s(sp_Time *t)    { return sp_Time_fmt(t, 0); }

/* ---- Complex arithmetic ---- */
/* All-zero Complex (defined below); every builder starts from it so the exact
   fields (exact / re_r / im_r) are never left as stack garbage. */
static sp_Complex sp_complex_zero(void);
/* Component-class (fl) propagation mirrors CRuby's numeric tower: add/sub act
   per component, so each Float bit carries through independently; mul/div mix
   all four components, so any Float input floats both results. */
/* Complex.polar: CRuby resolves an angle of exactly 0, pi/2, or pi without
   cos/sin (the magnitude keeps its class, the other component is 0.0);
   any other angle computes both components as Floats. m_is_f carries the
   magnitude's static class from codegen. */
sp_Complex sp_complex_polar(mrb_float m, mrb_float a, int m_is_f) {
  sp_Complex c = sp_complex_zero();
  int mf = m_is_f ? 1 : 0;
  if (a == 0)      { c.re = m;  c.im = 0.0; c.fl = (unsigned char)((mf ? SP_CPLX_RE_F : 0) | SP_CPLX_IM_F); return c; }
  if (a == M_PI_2) { c.re = 0.0; c.im = m;  c.fl = (unsigned char)(SP_CPLX_RE_F | (mf ? SP_CPLX_IM_F : 0)); return c; }
  if (a == M_PI)   { c.re = -m; c.im = 0.0; c.fl = (unsigned char)((mf ? SP_CPLX_RE_F : 0) | SP_CPLX_IM_F); return c; }
  c.re = m * cos(a); c.im = m * sin(a); c.fl = SP_CPLX_RE_F | SP_CPLX_IM_F;
  return c;
}
/* ---- exact Complex components ----
   A part is one of: FLOAT (fl bit), RATIONAL (exact bit, value in re_r/im_r),
   or INT (a whole float mirror). Arithmetic promotes per part: float wins;
   a rational operand keeps the result rational even when whole ((1/1), like
   MRI); int op int stays int. The float mirror is always refreshed so the
   float-only readers (abs/arg/polar) need no changes. */
typedef struct { unsigned char k; sp_Rational r; mrb_float f; } sp_cpart;  /* k: 0=int 1=float 2=rational */
static sp_cpart sp_cpart_get(sp_Complex c, int im) {
  sp_cpart p;
  p.f = im ? c.im : c.re;
  if (c.exact & (im ? SP_CPLX_IM_X : SP_CPLX_RE_X)) { p.k = 2; p.r = im ? c.im_r : c.re_r; }
  else if (c.fl & (im ? SP_CPLX_IM_F : SP_CPLX_RE_F)) { p.k = 1; p.r.num = 0; p.r.den = 1; }
  else { p.k = 0; p.r.num = (mrb_int)p.f; p.r.den = 1; }
  return p;
}
static void sp_cpart_put(sp_Complex *c, int im, sp_cpart p) {
  if (p.k == 2) p.f = sp_rational_to_f(p.r);
  if (im) c->im = p.f; else c->re = p.f;
  if (p.k == 1) { c->fl |= im ? SP_CPLX_IM_F : SP_CPLX_RE_F; }
  if (p.k == 2) {
    c->exact |= im ? SP_CPLX_IM_X : SP_CPLX_RE_X;
    if (im) c->im_r = p.r; else c->re_r = p.r;
  }
}
static sp_cpart sp_cpart_op(char op, sp_cpart a, sp_cpart b) {
  sp_cpart r;
  if (a.k == 1 || b.k == 1) {  /* any Float operand floats the result */
    r.k = 1; r.r.num = 0; r.r.den = 1;
    mrb_float x = a.k == 2 ? sp_rational_to_f(a.r) : a.f;
    mrb_float y = b.k == 2 ? sp_rational_to_f(b.r) : b.f;
    r.f = op == '+' ? x + y : op == '-' ? x - y : op == '*' ? x * y : x / y;
    return r;
  }
  sp_Rational x = a.r, y = b.r;
  sp_Rational v = op == '+' ? sp_rational_add(x, y)
                : op == '-' ? sp_rational_sub(x, y)
                : op == '*' ? sp_rational_mul(x, y)
                :             sp_rational_div(x, y);
  r.r = v; r.f = sp_rational_to_f(v);
  /* MRI canonicalization is operation-directed: Complex#/ demotes a whole
     result to Integer (Complex(1,2)/2 -> imag 1, not (1/1)), while a Rational
     operand under +/-/* keeps the result Rational even when whole (Integer *
     Rational -> (1/1)). int op int (no Rational, no divide) stays Integer. */
  if (op == '/') r.k = (v.den == 1) ? 0 : 2;
  else if (a.k == 2 || b.k == 2) r.k = 2;
  else r.k = 0;
  return r;
}
static int sp_complex_any_exact(sp_Complex a, sp_Complex b) { return (a.exact | b.exact) != 0; }
static sp_Complex sp_complex_zero(void) { sp_Complex c; memset(&c, 0, sizeof c); return c; }
sp_Complex sp_complex_from_rational(sp_Rational r) {
  sp_Complex c = sp_complex_zero();
  c.re = sp_rational_to_f(r);
  c.exact = SP_CPLX_RE_X;
  c.re_r = r;
  c.im_r.den = 1;
  return c;
}
sp_Complex sp_complex_add(sp_Complex a, sp_Complex b) {
  if (sp_complex_any_exact(a, b)) {
    sp_Complex c = sp_complex_zero();
    sp_cpart_put(&c, 0, sp_cpart_op('+', sp_cpart_get(a, 0), sp_cpart_get(b, 0)));
    sp_cpart_put(&c, 1, sp_cpart_op('+', sp_cpart_get(a, 1), sp_cpart_get(b, 1)));
    return c;
  }
  sp_Complex c = sp_complex_zero(); c.re = a.re + b.re; c.im = a.im + b.im; c.fl = a.fl | b.fl; return c; }
sp_Complex sp_complex_mul(sp_Complex a, sp_Complex b) {
  if (sp_complex_any_exact(a, b)) {
    sp_cpart ar = sp_cpart_get(a, 0), ai = sp_cpart_get(a, 1);
    sp_cpart br = sp_cpart_get(b, 0), bi = sp_cpart_get(b, 1);
    sp_Complex c = sp_complex_zero();
    sp_cpart_put(&c, 0, sp_cpart_op('-', sp_cpart_op('*', ar, br), sp_cpart_op('*', ai, bi)));
    sp_cpart_put(&c, 1, sp_cpart_op('+', sp_cpart_op('*', ar, bi), sp_cpart_op('*', ai, br)));
    return c;
  }
  sp_Complex c = sp_complex_zero(); c.re = (a.re * b.re) - (a.im * b.im); c.im = (a.re * b.im) + (a.im * b.re); c.fl = (a.fl | b.fl) ? (SP_CPLX_RE_F | SP_CPLX_IM_F) : 0; return c; }
/* Complex <*|/> a real operand: MRI scales each component by the real value
   rather than running the cross formula, so a Float zero in the (absent)
   imaginary part cannot pollute the other component's exactness
   (Complex(0.5,1)*Rational(1,2) -> (0.25+(1/2)*i), not (0.25+0.5i)). `r`
   carries the real operand in its real component (imaginary is 0). `op` is
   '*' or '/'. */
sp_Complex sp_complex_scale(sp_Complex a, sp_Complex r, char op) {
  sp_cpart s = sp_cpart_get(r, 0);
  sp_Complex c = sp_complex_zero();
  sp_cpart_put(&c, 0, sp_cpart_op(op, sp_cpart_get(a, 0), s));
  sp_cpart_put(&c, 1, sp_cpart_op(op, sp_cpart_get(a, 1), s));
  return c;
}
sp_Complex sp_complex_conjugate(sp_Complex a) { sp_Complex c = sp_complex_zero(); c.re = a.re; c.im = -a.im; c.fl = a.fl; return c; }
sp_Complex sp_complex_sub(sp_Complex a, sp_Complex b) {
  if (sp_complex_any_exact(a, b)) {
    sp_Complex c = sp_complex_zero();
    sp_cpart_put(&c, 0, sp_cpart_op('-', sp_cpart_get(a, 0), sp_cpart_get(b, 0)));
    sp_cpart_put(&c, 1, sp_cpart_op('-', sp_cpart_get(a, 1), sp_cpart_get(b, 1)));
    return c;
  }
  sp_Complex c = sp_complex_zero(); c.re = a.re - b.re; c.im = a.im - b.im; c.fl = a.fl | b.fl; return c; }
sp_Complex sp_complex_div(sp_Complex a, sp_Complex b) {
  mrb_float d = (b.re * b.re) + (b.im * b.im); sp_Complex c = sp_complex_zero();
  c.re = ((a.re * b.re) + (a.im * b.im)) / d; c.im = ((a.im * b.re) - (a.re * b.im)) / d;
  c.fl = (a.fl | b.fl) ? (SP_CPLX_RE_F | SP_CPLX_IM_F) : 0;
  return c;
}
/* Complex divided by a real scalar divides each component -- unlike the
   conjugate formula, this yields Infinity (not NaN) when the divisor is 0.0,
   matching MRI's Complex#/ against a Float. */
sp_Complex sp_complex_div_real(sp_Complex a, mrb_float b) {
  sp_Complex c = sp_complex_zero(); c.re = a.re / b; c.im = a.im / b; c.fl = SP_CPLX_RE_F | SP_CPLX_IM_F; return c;
}
/* Complex divided by an Integer follows integer zero-division rules: dividing
   by 0 raises ZeroDivisionError, as in MRI (a Float divisor gives Infinity).
   Each component divides exactly: an integer/rational part yields a reduced
   Rational (whole results demote to Integer, like MRI's Complex#/), while a
   Float part stays Float. */
sp_Complex sp_complex_div_int(sp_Complex a, mrb_int b) {
  if (b == 0) sp_raise_cls("ZeroDivisionError", "divided by 0");
  sp_cpart d; d.k = 0; d.f = (mrb_float)b; d.r.num = b; d.r.den = 1;
  sp_Complex c = sp_complex_zero();
  sp_cpart_put(&c, 0, sp_cpart_op('/', sp_cpart_get(a, 0), d));
  sp_cpart_put(&c, 1, sp_cpart_op('/', sp_cpart_get(a, 1), d));
  return c;
}
sp_Complex sp_complex_neg(sp_Complex a) { sp_Complex c = sp_complex_zero(); c.re = -a.re; c.im = -a.im; c.fl = a.fl; return c; }
mrb_float sp_complex_abs2(sp_Complex a) { return (a.re * a.re) + (a.im * a.im); }
mrb_float sp_complex_abs(sp_Complex a) { return sqrt((a.re * a.re) + (a.im * a.im)); }
mrb_bool sp_complex_eq(sp_Complex a, sp_Complex b) {
  if (a.exact || b.exact) {
    sp_cpart ar = sp_cpart_get(a, 0), br = sp_cpart_get(b, 0);
    sp_cpart ai = sp_cpart_get(a, 1), bi = sp_cpart_get(b, 1);
    /* a float part compares by mirror; non-float parts compare exactly */
    if ((ar.k == 1) != (br.k == 1) || (ai.k == 1) != (bi.k == 1))
      return a.re == b.re && a.im == b.im;
    if (ar.k != 1 && (ar.r.num * br.r.den != br.r.num * ar.r.den)) return FALSE;
    if (ai.k != 1 && (ai.r.num * bi.r.den != bi.r.num * ai.r.den)) return FALSE;
    if (ar.k == 1 && a.re != b.re) return FALSE;
    if (ai.k == 1 && a.im != b.im) return FALSE;
    return TRUE;
  }
  return a.re == b.re && a.im == b.im; }
/* z ** w for a non-integer exponent: exp(w * ln z); an integral real w
   defers to the exact integer power (preserving component classes). */
sp_Complex sp_complex_pow_c(sp_Complex z, sp_Complex w) {
  if (w.im == 0.0 && w.re == (mrb_float)(mrb_int)w.re && !(w.fl & SP_CPLX_RE_F))
    return sp_complex_pow(z, (mrb_int)w.re);
  mrb_float lr = 0.5 * log(z.re * z.re + z.im * z.im);
  mrb_float th = atan2(z.im, z.re);
  mrb_float xa = w.re * lr - w.im * th;
  mrb_float xb = w.re * th + w.im * lr;
  mrb_float m = exp(xa);
  sp_Complex r = sp_complex_zero();
  r.re = m * cos(xb);
  r.im = m * sin(xb);
  r.fl = SP_CPLX_RE_F | SP_CPLX_IM_F;
  return r;
}
sp_Complex sp_complex_pow(sp_Complex a, mrb_int e) {
  sp_Complex r = sp_complex_zero(); r.re = 1; r.im = 0; r.fl = 0;
  mrb_int k = e < 0 ? -e : e;
  for (mrb_int i = 0; i < k; i++) r = sp_complex_mul(r, a);
  if (e < 0) { sp_Complex one = sp_complex_zero(); one.re = 1; one.im = 0; one.fl = 0; r = sp_complex_div(one, r); }
  return r;
}

/* ---- Rational arithmetic ----
   Intermediate products use a wider type; a result that does not fit back into
   mrb_int raises RangeError (mruby promotes to Bigint -- a later phase can too).
   __int128 covers the 64-bit build; int64 covers two int32 operands losslessly. */
static mrb_int sp_rational_gcd_i(mrb_int a, mrb_int b) {
  if (a < 0) a = -a;
  if (b < 0) b = -b;
  while (b) { mrb_int t = b; b = a % b; a = t; }
  return a;
}
sp_Rational sp_rational_new(mrb_int n, mrb_int d) {
  sp_Rational r;
  if (d == 0) { r.num = n; r.den = 0; return r; }
  if (d < 0) { n = -n; d = -d; }
  mrb_int g = sp_rational_gcd_i(n, d);
  if (g <= 0) g = 1;
  r.num = n / g;
  r.den = d / g;
  return r;
}
/* String#to_r: parse a leading numeric of the form [ws][sign]digits[.digits][/digits],
   stopping at the first non-numeric byte; an unparseable string is 0/1 (MRI). */
sp_Rational sp_str_to_r(const char *s) {
  if (!s) return sp_rational_new(0, 1);
  const char *p = s;
  while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r' || *p == '\f' || *p == '\v') p++;
  mrb_int sign = 1;
  if (*p == '+') p++;
  else if (*p == '-') { sign = -1; p++; }
  mrb_int num = 0, den = 1;
  int any = 0;
  while (*p >= '0' && *p <= '9') { num = num * 10 + (*p - '0'); p++; any = 1; }
  if (*p == '.') {
    p++;
    while (*p >= '0' && *p <= '9') { num = num * 10 + (*p - '0'); den *= 10; p++; any = 1; }
  }
  if (*p == '/') {
    p++;
    mrb_int d2 = 0; int anyd = 0;
    while (*p >= '0' && *p <= '9') { d2 = d2 * 10 + (*p - '0'); p++; anyd = 1; }
    if (anyd && d2 != 0) den *= d2;
  }
  if (!any) return sp_rational_new(0, 1);
  return sp_rational_new(sign * num, den);
}
#if INTPTR_MAX > 0x7fffffff
typedef __int128 sp_rat_wide;
#else
typedef long long sp_rat_wide;
#endif
static mrb_int sp_rat_fit(sp_rat_wide v) {
  if (v > (sp_rat_wide)INTPTR_MAX || v < (sp_rat_wide)(-INTPTR_MAX))
    sp_raise_cls("RangeError", "Rational out of mrb_int range");
  return (mrb_int)v;
}
static sp_Rational sp_rational_new_wide(sp_rat_wide n, sp_rat_wide d) {
  if (d == 0) sp_raise_cls("ZeroDivisionError", "divided by 0");
  if (d < 0) { n = -n; d = -d; }
  sp_rat_wide a = n < 0 ? -n : n, b = d;
  while (b) { sp_rat_wide t = b; b = a % b; a = t; }
  if (a <= 0) a = 1;
  sp_Rational r;
  r.num = sp_rat_fit(n / a);
  r.den = sp_rat_fit(d / a);
  return r;
}
sp_Rational sp_rational_add(sp_Rational a, sp_Rational b) {
  return sp_rational_new_wide(((sp_rat_wide)a.num * b.den) + ((sp_rat_wide)b.num * a.den),
                              (sp_rat_wide)a.den * b.den);
}
sp_Rational sp_rational_sub(sp_Rational a, sp_Rational b) {
  return sp_rational_new_wide(((sp_rat_wide)a.num * b.den) - ((sp_rat_wide)b.num * a.den),
                              (sp_rat_wide)a.den * b.den);
}
sp_Rational sp_rational_mul(sp_Rational a, sp_Rational b) {
  return sp_rational_new_wide((sp_rat_wide)a.num * b.num, (sp_rat_wide)a.den * b.den);
}
sp_Rational sp_rational_div(sp_Rational a, sp_Rational b) {
  if (b.num == 0) sp_raise_cls("ZeroDivisionError", "divided by 0");
  return sp_rational_new_wide((sp_rat_wide)a.num * b.den, (sp_rat_wide)a.den * b.num);
}
sp_Rational sp_rational_neg(sp_Rational a) { a.num = -a.num; return a; }
sp_Rational sp_rational_abs(sp_Rational a) { if (a.num < 0) a.num = -a.num; return a; }
mrb_int sp_rational_cmp(sp_Rational a, sp_Rational b) {
  sp_rat_wide l = (sp_rat_wide)a.num * b.den, r = (sp_rat_wide)b.num * a.den;
  return l < r ? -1 : (l > r ? 1 : 0);
}
mrb_bool sp_rational_eq(sp_Rational a, sp_Rational b) {
  return a.num == b.num && a.den == b.den;
}
mrb_float sp_rational_to_f(sp_Rational a) {
  return (mrb_float)a.num / (mrb_float)a.den;
}
/* 2^e as a wide integer, raising RangeError past the mrb_int range. */
static sp_rat_wide sp_rat_pow2(int e) {
  sp_rat_wide r = 1;
  for (int i = 0; i < e; i++) {
    r <<= 1;
    if (r > (sp_rat_wide)INTPTR_MAX) sp_raise_cls("RangeError", "Rational out of mrb_int range");
  }
  return r;
}
/* Float#to_r : the exact rational value of the double. A finite double equals
   m * 2^e for a 53-bit integer mantissa m, so the conversion is exact. When the
   exact numerator/denominator does not fit in mrb_int (huge magnitude or a tiny
   subnormal), it raises RangeError -- spinel's Rational is int64/int64 and does
   not promote to a Bignum (matching the existing Rational-overflow behavior). */
sp_Rational sp_float_to_rational(mrb_float f) {
  if (isnan(f) || isinf(f)) sp_raise_cls("FloatDomainError", isnan(f) ? "NaN" : "Infinity");
  if (f == 0.0) { sp_Rational z; z.num = 0; z.den = 1; return z; }
  int e;
  double m = frexp((double)f, &e);          /* f = m * 2^e, 0.5 <= |m| < 1 */
  for (int i = 0; i < 53 && m != floor(m); i++) { m *= 2.0; e--; }
  sp_rat_wide num = (sp_rat_wide)m;          /* integer mantissa (fits in 54 bits) */
  if (e >= 0) {
    /* num * 2^e can exceed sp_rat_wide on a 32-bit build (long long); guard the
       product first. p > INTPTR_MAX/|num| is exactly "num*p won't fit mrb_int",
       so this raises rather than overflowing. */
    sp_rat_wide p = sp_rat_pow2(e), an = num < 0 ? -num : num;
    if (an != 0 && p > (sp_rat_wide)INTPTR_MAX / an)
      sp_raise_cls("RangeError", "Rational out of mrb_int range");
    return sp_rational_new_wide(num * p, 1);
  }
  return sp_rational_new_wide(num, sp_rat_pow2(-e));
}
/* Simplest p/q with lo <= p/q <= hi, for 0 < lo <= hi. Classic continued-fraction
   "simplest rational in an interval" recursion; the convergent depth is bounded by
   the interval, so q stays small for sane epsilons. */
static void sp_simplest_pos(double lo, double hi, sp_rat_wide *np, sp_rat_wide *dp) {
  double fl = floor(lo);
  if (fl == lo) { *np = (sp_rat_wide)fl; *dp = 1; return; }   /* lo is an integer */
  if (fl == floor(hi)) {                                       /* no integer in (lo,hi) */
    sp_rat_wide n, d;
    sp_simplest_pos(1.0 / (hi - fl), 1.0 / (lo - fl), &n, &d);
    *np = ((sp_rat_wide)fl * n) + d;
    *dp = n;
    return;
  }
  *np = (sp_rat_wide)fl + 1; *dp = 1;                          /* fl+1 lies in [lo,hi] */
}
static sp_Rational sp_rationalize_interval(double lo, double hi) {
  if (lo > hi) { double t = lo; lo = hi; hi = t; }
  if (lo <= 0.0 && hi >= 0.0) { sp_Rational z; z.num = 0; z.den = 1; return z; }
  int neg = 0;
  if (hi < 0.0) { double t = -lo; lo = -hi; hi = t; neg = 1; } /* fold to positives */
  /* Any p/q in [lo,hi] has p >= lo*q >= lo, so lo past mrb_int can't fit and
     floor(lo) would also overflow the (double->sp_rat_wide) cast below. */
  if (lo > (double)INTPTR_MAX) sp_raise_cls("RangeError", "Rational out of mrb_int range");
  sp_rat_wide n, d;
  sp_simplest_pos(lo, hi, &n, &d);
  if (neg) n = -n;
  return sp_rational_new_wide(n, d);
}
sp_Rational sp_float_rationalize(mrb_float f, mrb_float eps) {
  if (isnan(f) || isinf(f)) sp_raise_cls("FloatDomainError", isnan(f) ? "NaN" : "Infinity");
  double e = eps < 0 ? -eps : eps;
  return sp_rationalize_interval((double)f - e, (double)f + e);
}
/* No-arg rationalize: simplest rational that round-trips to this exact double,
   i.e. lying in the half-ulp interval around f. */
/* No-arg rationalize: the simplest rational whose nearest double is exactly f.
   The continued-fraction convergents of f are, by construction, the simplest
   rationals approximating it in increasing complexity, so the FIRST convergent
   that round-trips to f is the answer. This is robust where a double-precision
   half-ulp interval search is not: computing the interval bounds in `double`
   rounds them back to f for an exactly-representable value (collapsing the
   interval) and amplifies rounding at the ulp scale for others. */
sp_Rational sp_float_rationalize0(mrb_float f) {
  if (isnan(f) || isinf(f)) sp_raise_cls("FloatDomainError", isnan(f) ? "NaN" : "Infinity");
  if (f == 0.0) { sp_Rational z; z.num = 0; z.den = 1; return z; }
  int neg = f < 0.0;
  double x = neg ? -(double)f : (double)f;
  double v = x;
  sp_rat_wide h0 = 1, h1 = 0, k0 = 0, k1 = 1;  /* convergent num/den recurrences */
  sp_rat_wide num = 0, den = 1;
  int found = 0;
  for (int i = 0; i < 64 && !found; i++) {
    /* floor(v) out of sp_rat_wide range would make the cast below UB; any such
       convergent exceeds INTPTR_MAX anyway, so bail to the exact-ratio fallback. */
    if (v > (double)INTPTR_MAX) break;
    sp_rat_wide a = (sp_rat_wide)floor(v);
    /* guard the convergent against mrb_int overflow before forming it (all in
       integer arithmetic: a*h0 + h1 <= INTPTR_MAX). */
    if (a > 0 && (h0 > ((sp_rat_wide)INTPTR_MAX - h1) / a ||
                  k0 > ((sp_rat_wide)INTPTR_MAX - k1) / a))
      break;
    sp_rat_wide h = a * h0 + h1;
    sp_rat_wide k = a * k0 + k1;
    num = h; den = k;
    if (k != 0 && (double)h / (double)k == x) { found = 1; break; }
    h1 = h0; h0 = h; k1 = k0; k0 = k;
    double frac = v - a;
    if (frac == 0.0) break;
    v = 1.0 / frac;
  }
  if (!found) return sp_float_to_rational(f);  /* fallback: exact bit ratio */
  return sp_rational_new_wide(neg ? -num : num, den);
}
/* Rational#round with no digits: nearest integer, ties away from zero
   (CRuby's Rational#round default; den > 0 by construction). */
mrb_int sp_rational_round_i(sp_Rational a) {
  sp_rat_wide q = a.num / a.den, r = a.num % a.den;
  if (r != 0) {
    sp_rat_wide r2 = (r < 0 ? -r : r) * 2;
    if (r2 >= a.den) q += a.num < 0 ? -1 : 1;
  }
  return sp_rat_fit(q);
}
/* Rational#div: floor division to an Integer (CRuby Numeric#div). */
mrb_int sp_rational_idiv(sp_Rational a, sp_Rational b) {
  if (b.num == 0) sp_raise_cls("ZeroDivisionError", "divided by 0");
  sp_rat_wide n = (sp_rat_wide)a.num * b.den, d = (sp_rat_wide)a.den * b.num;
  if (d < 0) { n = -n; d = -d; }
  sp_rat_wide q = n / d;
  if (n % d != 0 && n < 0) q--;
  return sp_rat_fit(q);
}
/* 10^e as a wide integer, raising RangeError past the mrb_int range. */
static sp_rat_wide sp_rat_pow10(mrb_int e) {
  sp_rat_wide r = 1;
  for (mrb_int i = 0; i < e; i++) {
    if (r > (sp_rat_wide)INTPTR_MAX / 10)
      sp_raise_cls("RangeError", "Rational out of mrb_int range");
    r *= 10;
  }
  return r;
}
/* Rational#round/#truncate with a precision: scale by 10^nd, round or chop to
   an integer, scale back. nd <= 0 yields an integer-valued Rational (den 1);
   codegen realizes the Integer class from the literal precision. */
mrb_int sp_rational_floor_i(sp_Rational a) {
  return a.num >= 0 ? a.num / a.den : -((-a.num + a.den - 1) / a.den);
}
mrb_int sp_rational_ceil_i(sp_Rational a) {
  return a.num >= 0 ? (a.num + a.den - 1) / a.den : -((-a.num) / a.den);
}
sp_Rational sp_rational_round_prec(sp_Rational a, mrb_int nd) {
  sp_rat_wide p = sp_rat_pow10(nd < 0 ? -nd : nd);
  sp_Rational s = nd >= 0 ? sp_rational_new_wide((sp_rat_wide)a.num * p, a.den)
                          : sp_rational_new_wide(a.num, (sp_rat_wide)a.den * p);
  mrb_int q = sp_rational_round_i(s);
  return nd >= 0 ? sp_rational_new_wide(q, p) : sp_rational_new_wide((sp_rat_wide)q * p, 1);
}
sp_Rational sp_rational_mod(sp_Rational a, sp_Rational b) {
  mrb_int q = sp_rational_idiv(a, b);
  return sp_rational_sub(a, sp_rational_mul(b, sp_rational_new(q, 1)));
}
sp_Rational sp_rational_rem(sp_Rational a, sp_Rational b) {
  sp_Rational d = sp_rational_div(a, b);
  mrb_int q = d.num / d.den;   /* toward zero */
  return sp_rational_sub(a, sp_rational_mul(b, sp_rational_new(q, 1)));
}
sp_Rational sp_rational_floor_prec(sp_Rational a, mrb_int nd) {
  sp_rat_wide p = sp_rat_pow10(nd < 0 ? -nd : nd);
  sp_Rational s = nd >= 0 ? sp_rational_new_wide((sp_rat_wide)a.num * p, a.den)
                          : sp_rational_new_wide(a.num, (sp_rat_wide)a.den * p);
  mrb_int q = sp_rational_floor_i(s);
  return nd >= 0 ? sp_rational_new_wide(q, p) : sp_rational_new_wide((sp_rat_wide)q * p, 1);
}
sp_Rational sp_rational_ceil_prec(sp_Rational a, mrb_int nd) {
  sp_rat_wide p = sp_rat_pow10(nd < 0 ? -nd : nd);
  sp_Rational s = nd >= 0 ? sp_rational_new_wide((sp_rat_wide)a.num * p, a.den)
                          : sp_rational_new_wide(a.num, (sp_rat_wide)a.den * p);
  mrb_int q = sp_rational_ceil_i(s);
  return nd >= 0 ? sp_rational_new_wide(q, p) : sp_rational_new_wide((sp_rat_wide)q * p, 1);
}
sp_Rational sp_rational_truncate_prec(sp_Rational a, mrb_int nd) {
  sp_rat_wide p = sp_rat_pow10(nd < 0 ? -nd : nd);
  sp_Rational s = nd >= 0 ? sp_rational_new_wide((sp_rat_wide)a.num * p, a.den)
                          : sp_rational_new_wide(a.num, (sp_rat_wide)a.den * p);
  mrb_int q = s.num / s.den;  /* toward zero */
  return nd >= 0 ? sp_rational_new_wide(q, p) : sp_rational_new_wide((sp_rat_wide)q * p, 1);
}
static sp_rat_wide sp_rat_ipow(sp_rat_wide base, mrb_int e) {
  sp_rat_wide r = 1;
  for (mrb_int i = 0; i < e; i++) {
    r *= base;
    if (r > (sp_rat_wide)INTPTR_MAX || r < (sp_rat_wide)(-INTPTR_MAX))
      sp_raise_cls("RangeError", "Rational out of mrb_int range");
  }
  return r;
}
sp_Rational sp_rational_pow(sp_Rational a, mrb_int e) {
  if (e >= 0) return sp_rational_new_wide(sp_rat_ipow(a.num, e), sp_rat_ipow(a.den, e));
  if (a.num == 0) sp_raise_cls("ZeroDivisionError", "divided by 0");
  return sp_rational_new_wide(sp_rat_ipow(a.den, -e), sp_rat_ipow(a.num, -e));
}
