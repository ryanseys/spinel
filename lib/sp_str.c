/* sp_str.c -- cold String transforms (see sp_str.h).
 *
 * Leaf `const char*` operations moved out of sp_runtime.h so they compile
 * once into libspinel_rt.a instead of into every generated TU. They reach
 * the GC string heap via sp_alloc.h and the typed arrays via sp_array.h;
 * sp_str_crypt calls into lib/sp_crypto.c; sp_sprintf / sp_raise_* resolve
 * at the final link against the generated TU. */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include "sp_str.h"
#include "sp_crypto.h"   /* sp_crypto_hmac_sha256_b64url for sp_str_crypt */

int sp_utf8_set_has(const uint32_t*cps,size_t n,uint32_t cp){for(size_t i=0;i<n;i++)if(cps[i]==cp)return 1;return 0;}
mrb_int sp_str_casecmp(const char*a,const char*b){if(!a||!b)return a==b?0:(a?1:-1);for(;;){int ca=tolower((unsigned char)*a),cb=tolower((unsigned char)*b);if(ca!=cb)return ca<cb?-1:1;if(!*a)return 0;a++;b++;}}
mrb_bool sp_str_valid_encoding(const char*s){if(!s)return TRUE;const unsigned char*p=(const unsigned char*)s;while(*p){unsigned c=*p;if(c<0x80){p++;continue;}int extra;unsigned cp;unsigned min;if((c&0xE0)==0xC0){extra=1;cp=c&0x1F;min=0x80;}else if((c&0xF0)==0xE0){extra=2;cp=c&0x0F;min=0x800;}else if((c&0xF8)==0xF0){extra=3;cp=c&0x07;min=0x10000;}else return FALSE;p++;for(int i=0;i<extra;i++){if((*p&0xC0)!=0x80)return FALSE;cp=(cp<<6)|(*p&0x3F);p++;}if(cp<min)return FALSE;if(cp>=0xD800&&cp<=0xDFFF)return FALSE;if(cp>0x10FFFF)return FALSE;}return TRUE;}
const char*sp_str_field(const char*s,const char*sep,mrb_int n){
  size_t sl=strlen(sep);mrb_int cur=0;const char*p=s;
  if(sl==0)return sp_str_empty;
  while(cur<n){const char*f=strstr(p,sep);if(!f)return sp_str_empty;p=f+sl;cur++;}
  const char*end=strstr(p,sep);size_t len=end?((size_t)(end-p)):strlen(p);
  char*r=sp_str_alloc_raw(len+1);memcpy(r,p,len);r[len]=0;return r;}
/* Count fields in s split by sep (without allocating). */
mrb_int sp_str_field_count(const char*s,const char*sep){
  if(*s==0)return 0;
  size_t sl=strlen(sep);if(sl==0)return(mrb_int)strlen(s);
  mrb_int c=1;const char*p=s;while((p=strstr(p,sep))!=NULL){c++;p+=sl;}return c;}
const char*sp_str_concat(const char*a,const char*b){if(!a)a=sp_str_empty;if(!b)b=sp_str_empty;size_t la=sp_str_byte_len(a),lb=sp_str_byte_len(b);char*r=sp_str_alloc(la+lb);memcpy(r,a,la);memcpy(r+la,b,lb);return r;}
/* Issue #760: NULL src to memcpy is UB. Treat NULL as empty string. */
const char*sp_str_concat3(const char*a,const char*b,const char*c){if(!a)a="";if(!b)b="";if(!c)c="";size_t la=sp_str_byte_len(a),lb=sp_str_byte_len(b),lc=sp_str_byte_len(c);char*r=sp_str_alloc(la+lb+lc);memcpy(r,a,la);memcpy(r+la,b,lb);memcpy(r+la+lb,c,lc);return r;}
const char*sp_str_concat4(const char*a,const char*b,const char*c,const char*d){if(!a)a="";if(!b)b="";if(!c)c="";if(!d)d="";size_t la=sp_str_byte_len(a),lb=sp_str_byte_len(b),lc=sp_str_byte_len(c),ld=sp_str_byte_len(d);char*r=sp_str_alloc(la+lb+lc+ld);memcpy(r,a,la);memcpy(r+la,b,lb);memcpy(r+la+lb,c,lc);memcpy(r+la+lb+lc,d,ld);return r;}
/* Concatenate N strings into a single GC-managed buffer. */
/* Issue #760: NULL entries treated as empty strings. */
const char*sp_str_concat_arr(const char *const *parts,int n){size_t total=0;for(int i=0;i<n;i++)total+=sp_str_byte_len(parts[i]?parts[i]:"");char*r=sp_str_alloc(total);char*p=r;for(int i=0;i<n;i++){const char*s=parts[i]?parts[i]:"";size_t sl=sp_str_byte_len(s);memcpy(p,s,sl);p+=sl;}return r;}
const char*sp_str_inspect(const char*s){if(!s){char*r=sp_str_alloc_raw(4);r[0]='n';r[1]='i';r[2]='l';r[3]=0;return r;}size_t sl=sp_str_byte_len(s);size_t cap=sl*4+3;char*r=sp_str_alloc_raw(cap);size_t o=0;r[o++]='"';for(size_t i=0;i<sl;i++){unsigned char c=(unsigned char)s[i];if(c=='\\'||c=='"'){r[o++]='\\';r[o++]=c;}else if(c=='\n'){r[o++]='\\';r[o++]='n';}else if(c=='\t'){r[o++]='\\';r[o++]='t';}else if(c=='\r'){r[o++]='\\';r[o++]='r';}else if(c<0x20||c==0x7f){snprintf(r+o,5,"\\x%02X",c);o+=4;}else{r[o++]=(char)c;}}r[o++]='"';r[o]=0;sp_str_set_len(r,o);return r;}
const char*sp_str_upcase(const char*s){if(!s)return sp_str_empty;size_t l=strlen(s);char*r=sp_str_alloc_raw(l+1);for(size_t i=0;i<l;i++)r[i]=toupper((unsigned char)s[i]);r[l]=0;return r;}
const char*sp_str_downcase(const char*s){if(!s)return sp_str_empty;size_t l=strlen(s);char*r=sp_str_alloc_raw(l+1);for(size_t i=0;i<l;i++)r[i]=tolower((unsigned char)s[i]);r[l]=0;return r;}
const char*sp_str_swapcase(const char*s){if(!s)return sp_str_empty;size_t l=strlen(s);char*r=sp_str_alloc_raw(l+1);for(size_t i=0;i<l;i++){unsigned char c=(unsigned char)s[i];if(isupper(c))r[i]=tolower(c);else if(islower(c))r[i]=toupper(c);else r[i]=s[i];}r[l]=0;return r;}
const char*sp_str_dump(const char*s){
  if(!s)return sp_str_empty;
  size_t n=strlen(s);
  char*out=sp_str_alloc_raw(n*4+3);size_t oi=0;
  out[oi++]='"';
  for(size_t i=0;i<n;i++){
    unsigned char c=(unsigned char)s[i];
    if(c=='"'){out[oi++]='\\';out[oi++]='"';}
    else if(c=='\\'){out[oi++]='\\';out[oi++]='\\';}
    else if(c=='#'){out[oi++]='\\';out[oi++]='#';}
    else if(c=='\n'){out[oi++]='\\';out[oi++]='n';}
    else if(c=='\t'){out[oi++]='\\';out[oi++]='t';}
    else if(c=='\r'){out[oi++]='\\';out[oi++]='r';}
    else if(c=='\f'){out[oi++]='\\';out[oi++]='f';}
    else if(c=='\v'){out[oi++]='\\';out[oi++]='v';}
    else if(c=='\a'){out[oi++]='\\';out[oi++]='a';}
    else if(c=='\b'){out[oi++]='\\';out[oi++]='b';}
    else if(c==27){out[oi++]='\\';out[oi++]='e';}
    else if(c==0){out[oi++]='\\';out[oi++]='0';}
    else if(c<0x20){oi+=(size_t)sprintf(out+oi,"\\x%02X",c);}
    else{out[oi++]=(char)c;}
  }
  out[oi++]='"';out[oi]=0;return out;
}
const char*sp_str_delete_prefix(const char*s,const char*p){if(!s)return sp_str_empty;if(!p)return s;size_t sl=strlen(s),pl=strlen(p);if(pl<=sl&&memcmp(s,p,pl)==0){char*r=sp_str_alloc_raw(sl-pl+1);memcpy(r,s+pl,sl-pl+1);return r;}char*r=sp_str_alloc_raw(sl+1);memcpy(r,s,sl+1);return r;}
const char*sp_str_substr(const char*s,mrb_int start,mrb_int len){if(!s||len<=0){char*r=sp_str_alloc_raw(1);r[0]=0;return r;}if(start<0)start=0;char*r=sp_str_alloc_raw(len+1);memcpy(r,s+start,len);r[len]=0;return r;}
const char*sp_str_delete_suffix(const char*s,const char*p){if(!s)return sp_str_empty;if(!p)return s;size_t sl=strlen(s),pl=strlen(p);if(pl<=sl&&memcmp(s+sl-pl,p,pl)==0){char*r=sp_str_alloc_raw(sl-pl+1);memcpy(r,s,sl-pl);r[sl-pl]=0;return r;}char*r=sp_str_alloc_raw(sl+1);memcpy(r,s,sl+1);return r;}
const char*sp_str_strip(const char*s){if(!s)return sp_str_empty;size_t len=sp_str_byte_len(s);size_t a=0;while(a<len&&(isspace((unsigned char)s[a])||s[a]=='\0'))a++;size_t b=len;while(b>a&&(isspace((unsigned char)s[b-1])||s[b-1]=='\0'))b--;size_t n=b-a;char*r=sp_str_alloc(n);memcpy(r,s+a,n);r[n]=0;return r;}
const char*sp_str_chomp(const char*s){if(!s)return sp_str_empty;size_t l=strlen(s);if(l>=2&&s[l-2]=='\r'&&s[l-1]=='\n')l-=2;else if(l>0&&s[l-1]=='\n')l--;else if(l>0&&s[l-1]=='\r')l--;char*r=sp_str_alloc_raw(l+1);memcpy(r,s,l);r[l]=0;return r;}
const char *sp_str_chomp_sep(const char *s, const char *sep) {
  if (!s) return sp_str_empty;
  size_t l = strlen(s);
  if (!sep || !*sep) {
    /* Empty sep = paragraph mode: strip trailing \r\n pairs and
       standalone \n's, but NOT standalone \r's. A trailing \r that
       is not part of a \r\n pair stops the stripping. */
    while (l > 0) {
      if (l >= 2 && s[l-2] == '\r' && s[l-1] == '\n') { l -= 2; continue; }
      if (s[l-1] == '\n') { l--; continue; }
      break;
    }
  }
else {
    size_t sl = strlen(sep);
    if (sl <= l && memcmp(s + l - sl, sep, sl) == 0) l -= sl;
  }
  char *r = sp_str_alloc_raw(l + 1);
  memcpy(r, s, l);
  r[l] = 0;
  return r;
}
const char*sp_str_chop(const char*s){if(!s)return sp_str_empty;size_t l=strlen(s);if(l>0){if(l>=2&&s[l-2]=='\r'&&s[l-1]=='\n')l-=2;else l--;}char*r=sp_str_alloc_raw(l+1);memcpy(r,s,l);r[l]=0;return r;}
mrb_bool sp_str_include(const char*s,const char*sub){if(!sub)sp_raise_cls("TypeError","no implicit conversion of nil into String");if(!s)return FALSE;return strstr(s,sub)!=NULL;}
mrb_bool sp_str_start_with(const char*s,const char*p){if(!p)sp_raise_cls("TypeError","no implicit conversion of nil into String");if(!s)return FALSE;return strncmp(s,p,strlen(p))==0;}
mrb_bool sp_str_end_with(const char*s,const char*suf){if(!suf)sp_raise_cls("TypeError","no implicit conversion of nil into String");if(!s)return FALSE;size_t ls=strlen(s),lsuf=strlen(suf);if(lsuf>ls)return FALSE;return strcmp(s+ls-lsuf,suf)==0;}
/* partition: [before, sep, after] at the first sep; no match -> [s, "", ""]. */
sp_StrArray *sp_str_partition(const char *s, const char *sep) {
  SP_GC_ROOT(s); SP_GC_ROOT(sep);
  sp_StrArray *r = sp_StrArray_new();
  mrb_int bl = (mrb_int)sp_str_byte_len(s), sl = (mrb_int)strlen(sep);
  const char *f = sl > 0 ? strstr(s, sep) : s;
  if (!f) { sp_StrArray_push(r, s); sp_StrArray_push(r, sp_str_empty); sp_StrArray_push(r, sp_str_empty); return r; }
  mrb_int pre = (mrb_int)(f - s);
  sp_StrArray_push(r, sp_str_byteslice(s, 0, pre));
  sp_StrArray_push(r, sp_str_byteslice(s, pre, sl));
  sp_StrArray_push(r, sp_str_byteslice(s, pre + sl, bl - pre - sl));
  return r;
}
/* rpartition: split at the last sep; no match -> ["", "", s]. */
sp_StrArray *sp_str_rpartition(const char *s, const char *sep) {
  SP_GC_ROOT(s); SP_GC_ROOT(sep);
  sp_StrArray *r = sp_StrArray_new();
  mrb_int bl = (mrb_int)sp_str_byte_len(s), sl = (mrb_int)strlen(sep);
  const char *last = NULL;
  if (sl > 0) { const char *p = s; while ((p = strstr(p, sep))) { last = p; p++; } }
  if (!last) { sp_StrArray_push(r, sp_str_empty); sp_StrArray_push(r, sp_str_empty); sp_StrArray_push(r, s); return r; }
  mrb_int pre = (mrb_int)(last - s);
  sp_StrArray_push(r, sp_str_byteslice(s, 0, pre));
  sp_StrArray_push(r, sp_str_byteslice(s, pre, sl));
  sp_StrArray_push(r, sp_str_byteslice(s, pre + sl, bl - pre - sl));
  return r;
}
sp_StrArray*sp_str_lines(const char*s){sp_StrArray*a=sp_StrArray_new();if(*s==0)return a;const char*end=s+strlen(s);const char*p=s;while(p<end){const char*nl=strchr(p,'\n');size_t n=nl?(size_t)(nl-p+1):(size_t)(end-p);char*r=sp_str_alloc_raw(n+1);memcpy(r,p,n);r[n]=0;sp_StrArray_push(a,r);if(!nl)break;p=nl+1;}return a;}
sp_StrArray*sp_str_lines_chomp(const char*s){sp_StrArray*a=sp_StrArray_new();if(*s==0)return a;const char*end=s+strlen(s);const char*p=s;while(p<end){const char*nl=strchr(p,'\n');size_t n=nl?(size_t)(nl-p):(size_t)(end-p);if(nl&&nl>s&&nl[-1]=='\r')n--;char*r=sp_str_alloc_raw(n+1);memcpy(r,p,n);r[n]=0;sp_StrArray_push(a,r);if(!nl)break;p=nl+1;}return a;}
const char*sp_str_byteslice(const char*s,mrb_int start,mrb_int len){mrb_int bl=(mrb_int)sp_str_byte_len(s);if(start<0)start+=bl;if(start<0||start>bl||len<0){return &("\xff" "")[1];}if(start+len>bl)len=bl-start;if(len<=0){return &("\xff" "")[1];}char*r=sp_str_alloc_raw(len+1);memcpy(r,s+start,len);r[len]=0;return r;}
/* String#ascii_only?: 1 iff every byte is in the 7-bit ASCII range. */
int sp_str_ascii_only(const char*s){mrb_int bl=(mrb_int)sp_str_byte_len(s);for(mrb_int i=0;i<bl;i++){if((unsigned char)s[i]>=0x80)return 0;}return 1;}
const char*sp_str_format_strarr(const char*fmt,sp_StrArray*a){size_t cap=strlen(fmt)+64;char*buf=(char*)malloc(cap);if(!buf){perror("malloc");exit(1);}size_t out=0;mrb_int idx=0;const char*p=fmt;while(*p){if(*p=='%'){if(p[1]=='s'){const char*s=(idx<a->len)?a->data[idx]:"";size_t sl=strlen(s);if(out+sl>=cap){size_t nc=(out+sl)*2+1;char*nb=(char*)realloc(buf,nc);if(!nb){free(buf);perror("realloc");exit(1);}buf=nb;cap=nc;}memcpy(buf+out,s,sl);out+=sl;idx++;p+=2;}else if(p[1]=='%'){if(out+1>=cap){size_t nc=cap*2;char*nb=(char*)realloc(buf,nc);if(!nb){free(buf);perror("realloc");exit(1);}buf=nb;cap=nc;}buf[out++]='%';p+=2;}else{if(out+1>=cap){size_t nc=cap*2;char*nb=(char*)realloc(buf,nc);if(!nb){free(buf);perror("realloc");exit(1);}buf=nb;cap=nc;}buf[out++]=*p++;}}else{if(out+1>=cap){size_t nc=cap*2;char*nb=(char*)realloc(buf,nc);if(!nb){free(buf);perror("realloc");exit(1);}buf=nb;cap=nc;}buf[out++]=*p++;}}buf[out]=0;char*r=sp_str_alloc(out);memcpy(r,buf,out);free(buf);return r;}
const char*sp_str_sub(const char*s,const char*pat,const char*rep){if(!s)return sp_str_empty;if(!pat||!rep)return s;const char*f=strstr(s,pat);if(!f)return s;size_t pl=strlen(pat),rl=strlen(rep),sl=strlen(s);char*r=sp_str_alloc_raw(sl-pl+rl+1);size_t n=f-s;memcpy(r,s,n);memcpy(r+n,rep,rl);memcpy(r+n+rl,f+pl,sl-n-pl+1);return r;}
const char*sp_str_capitalize(const char*s){if(!s)return sp_str_empty;size_t l=strlen(s);char*r=sp_str_alloc_raw(l+1);for(size_t i=0;i<=l;i++)r[i]=tolower((unsigned char)s[i]);if(l>0)r[0]=toupper((unsigned char)r[0]);return r;}
const char*sp_str_repeat(const char*s,mrb_int n){
  if(n<0) sp_raise_cls("ArgumentError","negative argument");
  if(!s||n<=0)return sp_str_empty;
  size_t l=strlen(s);
  if(l==0) return sp_str_empty;
  if((size_t)n>SIZE_MAX/l) sp_raise_cls("ArgumentError","string size too big");
  size_t total=(size_t)n*l;
  if(total>(size_t)(1u<<30)) sp_raise_cls("ArgumentError","string size too big");
  char*r=sp_str_alloc_raw(total+1);
  for(mrb_int i=0;i<n;i++)memcpy(r+l*i,s,l);
  r[total]=0;
  return r;
}
sp_IntArray*sp_str_bytes(const char*s){sp_IntArray*a=sp_IntArray_new();if(!s)return a;size_t n=sp_str_byte_len(s);for(size_t i=0;i<n;i++)sp_IntArray_push(a,(mrb_int)(unsigned char)s[i]);return a;}
const char *sp_str_crypt(const char *s, const char *salt) {
  if (!salt) salt = "";
  char salt2[3];
  salt2[0] = salt[0] ? salt[0] : '.';
  salt2[1] = (salt[0] && salt[1]) ? salt[1] : '.';
  salt2[2] = 0;
  const char *digest = sp_crypto_hmac_sha256_b64url(salt2, s ? s : "");
  char *r = sp_str_alloc(13);
  r[0] = salt2[0];
  r[1] = salt2[1];
  for (int i = 0; i < 11; i++) {
    char c = digest[i];
    /* Map b64url's `-`/`_` to crypt-alphabet `.`/`/` so the
       output stays in `[./0-9A-Za-z]` like the historical
       crypt result. */
    if (c == '-') c = '.';
    else if (c == '_') c = '/';
    r[2 + i] = c;
  }
  r[13] = 0;
  sp_str_set_len(r, 13);
  return r;
}
const char*sp_str_lstrip(const char*s){if(!s)return sp_str_empty;size_t len=sp_str_byte_len(s);size_t a=0;while(a<len&&(isspace((unsigned char)s[a])||s[a]=='\0'))a++;size_t n=len-a;char*r=sp_str_alloc(n);memcpy(r,s+a,n);r[n]=0;return r;}
const char*sp_str_rstrip(const char*s){if(!s)return sp_str_empty;size_t len=sp_str_byte_len(s);size_t b=len;while(b>0&&(isspace((unsigned char)s[b-1])||s[b-1]=='\0'))b--;char*r=sp_str_alloc(b);memcpy(r,s,b);r[b]=0;return r;}
const char*sp_str_dup(const char*s){if(!s)return NULL;size_t l=strlen(s);char*r=sp_str_alloc_raw(l+1);memcpy(r,s,l+1);return r;}
