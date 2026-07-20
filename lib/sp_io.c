/* sp_io.c -- File / IO handle ops in libspinel_rt.a.
 *
 * The allocation-free handle ops (open / pipe / fdopen / write / close /
 * closed? / puts / print / flush / eof?); the string-returning readers
 * (gets / read / read_n / path) stay inline in sp_runtime.h.
 *
 * Self-contained: includes sp_io.h (the sp_File layout) + sp_gc.h
 * (sp_mark_string), not sp_runtime.h. */
#include "sp_io.h"
#include "sp_gc.h"   /* sp_mark_string */
#include <stdlib.h>
#include <string.h>
#include <unistd.h>   /* pipe, isatty */
#include <sys/stat.h> /* stat() for the File predicates */
#include <sys/ioctl.h> /* TIOCGWINSZ for #winsize */
#include <sys/time.h> /* utimes() for File.utime */
#include <errno.h>

/* Provided by the generated TU / libspinel_rt.a. */
extern void *sp_gc_alloc(size_t sz, void (*fin)(void *), void (*scn)(void *));
extern SP_NORETURN void sp_raise_cls(const char *cls, const char *msg);
extern const char *sp_sprintf(const char *fmt, ...);

static void sp_File_fin(void *p) { sp_File *f = (sp_File *)p; if (f->fp) { fclose(f->fp); f->fp = NULL; } }
static void sp_File_scan(void *p) { sp_File *f = (sp_File *)p; if (f->path) sp_mark_string(f->path); if (f->mode) sp_mark_string(f->mode); }

sp_File *sp_File_open(const char *path, const char *mode) {
  sp_File *f = (sp_File *)sp_gc_alloc(sizeof(sp_File), sp_File_fin, sp_File_scan);
  f->fp = fopen(path ? path : "", mode ? mode : "r");
  if (!f->fp) { sp_raise_cls("Errno::ENOENT", "No such file or directory"); return NULL; }
  f->path = path;
  f->mode = mode;
  f->lineno = 0;
  return f;
}

/* Returns 0 on success, -1 on error. */
int sp_io_make_pipe(int fds[2]) {
  return pipe(fds);
}

/* IO.pipe end: wrap a raw pipe fd in a GC-managed sp_File so the
   sp_File_* I/O ops work on it. Same finalizer/scan as sp_File_open. */
sp_File *sp_io_fdopen(int fd, const char *mode) {
  sp_File *f = (sp_File *)sp_gc_alloc(sizeof(sp_File), sp_File_fin, sp_File_scan);
  f->fp = fdopen(fd, mode ? mode : "r");
  if (!f->fp) { sp_raise_cls("IOError", "fdopen failed"); return NULL; }
  f->path = NULL;
  f->mode = mode;
  f->lineno = 0;
  return f;
}

mrb_int sp_File_write(sp_File *f, const char *s) {
  if (!f || !f->fp || !s) return 0;
  size_t n = strlen(s);
  return (mrb_int)fwrite(s, 1, n, f->fp);
}

mrb_bool sp_File_tty_p(sp_File *f) {
  return (f && f->fp && isatty(fileno(f->fp))) ? 1 : 0;
}

mrb_int sp_File_fileno(sp_File *f) {
  return (f && f->fp) ? (mrb_int)fileno(f->fp) : -1;
}

/* IO#winsize -> [rows, cols]. Queries the terminal; a non-tty (pipe/file) has
   no size, so CRuby raises there, but returning [0, 0] keeps the common
   "STDOUT.winsize" probe compiling and running without an exception path. */
sp_IntArray *sp_File_winsize(sp_File *f) {
  mrb_int rows = 0, cols = 0;
  if (f && f->fp) {
    struct winsize ws;
    if (ioctl(fileno(f->fp), TIOCGWINSZ, &ws) == 0) { rows = ws.ws_row; cols = ws.ws_col; }
  }
  sp_IntArray *a = sp_IntArray_new();
  sp_IntArray_push(a, rows);
  sp_IntArray_push(a, cols);
  return a;
}

sp_File *sp_io_stdout(void) {
  static sp_File f = { NULL, "<STDOUT>", "w" };
  if (!f.fp) f.fp = stdout;
  return &f;
}

sp_File *sp_io_stderr(void) {
  static sp_File f = { NULL, "<STDERR>", "w" };
  if (!f.fp) f.fp = stderr;
  return &f;
}

sp_File *sp_io_stdin(void) {
  static sp_File f = { NULL, "<STDIN>", "r" };
  if (!f.fp) f.fp = stdin;
  return &f;
}

mrb_int sp_File_close(sp_File *f) {
  /* never fclose the shared stdout/stderr handles (sp_io_stdout/sp_io_stderr):
     closing the process's standard streams would corrupt the singleton and any
     later write through it. Closing them is a no-op. */
  if (f && f->fp && f->fp != stdout && f->fp != stderr && f->fp != stdin) { fclose(f->fp); f->fp = NULL; }
  return 0;
}

mrb_bool sp_File_closed_p(sp_File *f) {
  return !f || !f->fp;
}

void sp_File_puts(sp_File *f, const char *s) {
  if (!f || !f->fp || !s) return;
  size_t n = strlen(s);
  fputs(s, f->fp);
  if (n == 0 || s[n - 1] != '\n') fputc('\n', f->fp);
}

void sp_File_print(sp_File *f, const char *s) {
  if (!f || !f->fp || !s) return;
  fputs(s, f->fp);
}

mrb_int sp_File_flush(sp_File *f) {
  if (f && f->fp) fflush(f->fp);
  return 0;
}

mrb_bool sp_File_eof_p(sp_File *f) {
  if (!f || !f->fp) return TRUE;
  int c = fgetc(f->fp);
  if (c == EOF) return TRUE;
  ungetc(c, f->fp);
  return FALSE;
}

mrb_int sp_File_seek(sp_File *f, mrb_int off, mrb_int whence) {
  if (!f || !f->fp) return -1;
  /* whence uses the Ruby IO::SEEK_* values (0/1/2), mapped explicitly so we
     never depend on the platform's SEEK_SET/CUR/END numbering. fseeko/ftello
     take off_t rather than fseek's long, so offsets past 2GB survive even
     where long is 32-bit. */
  int w = (whence == 1) ? SEEK_CUR : (whence == 2) ? SEEK_END : SEEK_SET;
  return (mrb_int)fseeko(f->fp, (off_t)off, w);
}

mrb_int sp_File_tell(sp_File *f) {
  if (!f || !f->fp) return -1;
  return (mrb_int)ftello(f->fp);
}

mrb_int sp_File_rewind(sp_File *f) {
  if (!f || !f->fp) return -1;
  rewind(f->fp);
  return 0;
}

/* ---- File metadata predicates ----
   libc / WinAPI only, no spinel-string allocation and no shared mutable
   state, so they live here rather than inline in sp_runtime.h. */
mrb_bool sp_file_directory(const char *path) {
  struct stat st;
  return path && stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

mrb_bool sp_file_file(const char *path) {
  struct stat st;
  return path && stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

mrb_bool sp_file_symlink(const char *path) {
  struct stat st;
  return path && lstat(path, &st) == 0 && S_ISLNK(st.st_mode);
}

/* map errno to the matching Errno:: class the way sp_cold.c's File ops do */
SP_NORETURN static void sp_file_raise_errno(const char *op, const char *path) {
  sp_raise_cls(errno == ENOENT ? "Errno::ENOENT" :
               errno == EACCES ? "Errno::EACCES" :
               errno == EEXIST ? "Errno::EEXIST" :
               errno == EPERM  ? "Errno::EPERM"  : "SystemCallError",
               sp_sprintf("%s @ %s - %s", strerror(errno), op, path ? path : ""));
}

mrb_bool sp_file_owned(const char *path) {
  struct stat st;
  return path && stat(path, &st) == 0 && st.st_uid == geteuid();
}
mrb_bool sp_file_grpowned(const char *path) {
  struct stat st;
  return path && stat(path, &st) == 0 && st.st_gid == getegid();
}
mrb_bool sp_file_setuid(const char *path) {
  struct stat st;
  return path && stat(path, &st) == 0 && (st.st_mode & S_ISUID) != 0;
}
mrb_bool sp_file_setgid(const char *path) {
  struct stat st;
  return path && stat(path, &st) == 0 && (st.st_mode & S_ISGID) != 0;
}
mrb_bool sp_file_sticky(const char *path) {
  struct stat st;
  return path && stat(path, &st) == 0 && (st.st_mode & S_ISVTX) != 0;
}
mrb_bool sp_file_socket(const char *path) {
  struct stat st;
  return path && stat(path, &st) == 0 && S_ISSOCK(st.st_mode);
}
mrb_bool sp_file_blockdev(const char *path) {
  struct stat st;
  return path && stat(path, &st) == 0 && S_ISBLK(st.st_mode);
}
mrb_bool sp_file_chardev(const char *path) {
  struct stat st;
  return path && stat(path, &st) == 0 && S_ISCHR(st.st_mode);
}
/* world_readable? / world_writable?: the permission bits (0..0777) when the
   other-read / other-write bit is set, else nil (SP_INT_NIL) (#3005) */
mrb_int sp_file_world_readable(const char *path) {
  struct stat st;
  if (!(path && stat(path, &st) == 0 && (st.st_mode & S_IROTH))) return SP_INT_NIL;
  return (mrb_int)(st.st_mode & 0777);
}
mrb_int sp_file_world_writable(const char *path) {
  struct stat st;
  if (!(path && stat(path, &st) == 0 && (st.st_mode & S_IWOTH))) return SP_INT_NIL;
  return (mrb_int)(st.st_mode & 0777);
}
mrb_int sp_file_do_symlink(const char *oldp, const char *newp) {
  if (symlink(oldp, newp) != 0) sp_file_raise_errno("symlink", newp);
  return 0;
}
mrb_int sp_file_do_link(const char *oldp, const char *newp) {
  if (link(oldp, newp) != 0) sp_file_raise_errno("link", newp);
  return 0;
}
mrb_int sp_file_umask(mrb_int mask, int have_arg) {
  if (have_arg) return (mrb_int)umask((mode_t)mask);
  mode_t cur = umask(0);   /* read is destructive; restore immediately */
  umask(cur);
  return (mrb_int)cur;
}
mrb_int sp_file_mkfifo(const char *path, mrb_int mode) {
  if (mkfifo(path, (mode_t)mode) != 0) sp_file_raise_errno("mkfifo", path);
  return 0;
}
mrb_int sp_file_utime(double atime, double mtime, const char *path) {
  struct timeval tv[2];
  tv[0].tv_sec = (time_t)atime; tv[0].tv_usec = (long)((atime - (double)(time_t)atime) * 1e6);
  tv[1].tv_sec = (time_t)mtime; tv[1].tv_usec = (long)((mtime - (double)(time_t)mtime) * 1e6);
  if (utimes(path, tv) != 0) sp_file_raise_errno("utime", path);
  return 1;
}

/* stat, not fopen: opening a FIFO for read blocks until a writer appears, so
   the old fopen probe hung File.exist? on a fresh mkfifo path (#3118). stat
   also answers true for directories, matching CRuby. */
mrb_bool sp_file_exist(const char *path) { struct stat st; return path && stat(path, &st) == 0; }
void sp_file_delete(const char *path) { remove(path); }
void sp_file_rename(const char *from, const char *to) { rename(from, to); }
