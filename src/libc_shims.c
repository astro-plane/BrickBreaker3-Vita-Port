/* libc-level shims for the iOS armv7 guest.
 * Guest ABI notes: iOS armv7 is softfp (floats/doubles in r0-r3) -> SOFT functions use the base AAPCS
 * even though the Vita build is hard-float. va_list on iOS is a plain pointer to the arg area. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdarg.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdint.h>

#ifdef __arm__
#define SOFT __attribute__((pcs("aapcs")))
#else
#define SOFT
#endif

/* GAME_DIR comes from game.h */

/* guest va_list (void*) -> host va_list (struct {void *__ap}) */
#ifdef __arm__
#define GUEST_VA(ap, g) va_list ap; *(void **)&ap = (g)
#else
#define GUEST_VA(ap, g) va_list ap; (void)(g); abort()
#endif

static char pathbuf[1024];
static const char *map_path(const char *p) {
    if (p && !strcmp(p, "log.txt")) p = "game_log.txt";      /* keep the game's own log apart from loader.txt */
    if (!p || strchr(p, ':') || p[0] == '/') return p;       /* already a Vita path */
    snprintf(pathbuf, sizeof pathbuf, GAME_DIR "%s", p);
    return pathbuf;
}

/* ---- stdio ---- */
static int w_puts(const char *s) { return puts(s); }
static int w_printf(const char *f, ...) { va_list a; va_start(a, f); int r = vprintf(f, a); va_end(a); return r; }
static int w_fprintf(FILE *fp, const char *f, ...) { va_list a; va_start(a, f); int r = vfprintf(fp, f, a); va_end(a); return r; }
static int w_sprintf(char *s, const char *f, ...) { va_list a; va_start(a, f); int r = vsprintf(s, f, a); va_end(a);
    { static volatile int cnt; if (cnt < 80) { cnt++; uint32_t ra = (uint32_t)(uintptr_t)__builtin_return_address(0); if (ra >= 0x98000000u && ra < 0x98600000u) ra = ra - 0x98000000u + 0x1000u;
        fprintf(stderr, "[sprintf] caller=vm%x fmt='%.60s' -> '%.100s'\n", ra, f, s); fflush(stderr); } }
    return r; }
static int w___sprintf_chk(char *s, int flag, size_t n, const char *f, ...) { (void)flag; va_list a; va_start(a, f); int r = vsnprintf(s, n, f, a); va_end(a); return r; }
static char *w___strcpy_chk(char *d, const char *s, size_t n) { (void)n; return strcpy(d, s); }
static int w_sscanf(const char *s, const char *f, ...) { va_list a; va_start(a, f); int r = vsscanf(s, f, a); va_end(a); return r; }
static int w_vprintf(const char *f, void *g) { GUEST_VA(a, g); return vprintf(f, a); }
static int w_vfprintf(FILE *fp, const char *f, void *g) { GUEST_VA(a, g); return vfprintf(fp, f, a); }
static int w_vsprintf(char *s, const char *f, void *g) { GUEST_VA(a, g); return vsprintf(s, f, a); }
static int w_vsnprintf(char *s, size_t n, const char *f, void *g) { GUEST_VA(a, g); return vsnprintf(s, n, f, a); }
static int w_vfscanf(FILE *fp, const char *f, void *g) { GUEST_VA(a, g); return vfscanf(fp, f, a); }
/* The game's big data archives (*.bar) are read with many small seeks; the memory card is slow at that (menu loads
 * stalled 1.5 s). Read-only opens of a .bar file are served from RAM: the whole file is loaded once, kept, and each
 * fopen gets an fmemopen stream over it. vita.cfg barcache=0 turns it off. */
int g_barcache = 1;
static struct { char path[160]; void *buf; size_t len; } g_bars[12]; static int g_nbars;
static FILE *bar_open(const char *mp) {
    size_t l = strlen(mp);
    if (!g_barcache || l < 5 || strcmp(mp + l - 4, ".bar")) return NULL;
    for (int i = 0; i < g_nbars; i++) if (!strcmp(g_bars[i].path, mp)) return fmemopen(g_bars[i].buf, g_bars[i].len, "rb");
    if (g_nbars >= 12 || l >= sizeof g_bars[0].path) return NULL;
    FILE *f = fopen(mp, "rb"); if (!f) return NULL;
    fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
    if (n <= 0 || n > 48 * 1024 * 1024) { fclose(f); return NULL; }
    void *b = malloc((size_t)n); if (!b) { fclose(f); return NULL; }
    setvbuf(f, NULL, _IOFBF, 1024 * 1024);
    size_t got = fread(b, 1, (size_t)n, f); fclose(f);
    if (got != (size_t)n) { free(b); return NULL; }
    strcpy(g_bars[g_nbars].path, mp); g_bars[g_nbars].buf = b; g_bars[g_nbars].len = (size_t)n; g_nbars++;
    fprintf(stderr, "[barcache] %s: %ld KB now served from RAM\n", mp, n >> 10); fflush(stderr);
    return fmemopen(b, (size_t)n, "rb");
}
static FILE *w_fopen(const char *p, const char *m) {
    const char *mp = map_path(p);
    FILE *f = NULL;
    if (mp && m && m[0] == 'r' && !strchr(m, '+')) f = bar_open(mp);
    int inram = f != NULL;
    if (!f) f = fopen(mp, m);
#ifndef NO_FILE_TRACE
    if (f && p && !strcmp(p, "log.txt")) setvbuf(f, NULL, _IONBF, 0);      /* game log: keep what was written before a crash */
    { static int n; if (n++ < 300) { fprintf(stderr, "[fopen] %s (%s) -> %s\n", mp ? mp : "(null)", m ? m : "?", f ? "ok" : "FAIL"); fflush(stderr); } }
#endif
    if (f && !inram && m && m[0] == 'r' && !(p && !strcmp(p, "log.txt"))) setvbuf(f, NULL, _IOFBF, 64 * 1024);   /* data files: read in 64 KB chunks (each memory-card read is slow) */
    return f;
}

/* ---- strings / memory ---- */
static void w_memset_pattern16(void *b, const void *pat, size_t len) {
    for (size_t i = 0; i < len; i++) ((uint8_t *)b)[i] = ((const uint8_t *)pat)[i & 15];
}
static double SOFT w_strtod(const char *s, char **e) { return strtod(s, e); }

/* ---- time (guest time_t / timeval are 32-bit) ---- */
volatile unsigned g_gtod_count; extern volatile unsigned g_req_sleep_last, g_req_sleep_n;
/* caller histogram: which guest code reads the clock / sleeps (return address = the guest call site) */
typedef struct { uint32_t lr; unsigned n; } CallerHist;
static CallerHist g_ch_tod[24], g_ch_slp[24]; static unsigned g_ch_calls;
static void ch_note(CallerHist *h, uint32_t lr) {
    for (int i = 0; i < 24; i++) { if (h[i].lr == lr) { h[i].n++; return; } if (!h[i].lr) { h[i].lr = lr; h[i].n = 1; return; } } }
static void ch_dump(void) {
    if (++g_ch_calls != 20000) return;
    fprintf(stderr, "[clock] gettimeofday callers (vm address: count):");
    for (int i = 0; i < 24 && g_ch_tod[i].lr; i++) fprintf(stderr, " %x:%u", g_ch_tod[i].lr - 0x98000000u + 0x1000u, g_ch_tod[i].n);
    fprintf(stderr, "\n[clock] usleep callers:");
    for (int i = 0; i < 24 && g_ch_slp[i].lr; i++) fprintf(stderr, " %x:%u", g_ch_slp[i].lr - 0x98000000u + 0x1000u, g_ch_slp[i].n);
    fprintf(stderr, "\n"); fflush(stderr);
}
static uint32_t caller_of_frame(uint32_t r7) {   /* guest ARM frame: push {r7, lr}; mov r7, sp -> [r7 + 4] = caller */
    if (r7 < 0x81000000u || r7 >= 0xa0000000u || (r7 & 3)) return 0;
    uint32_t lr = *(volatile uint32_t *)(uintptr_t)(r7 + 4);
    return (lr >= 0x98000000u && lr < 0x98600000u) ? lr : 0;
}
__attribute__((used)) int w_gettimeofday_r7(int32_t *tv, void *tz, uint32_t r7);
__attribute__((naked, noinline)) static int w_gettimeofday(int32_t *tv, void *tz) { __asm__ volatile(".syntax unified\nmov r2, r7\nb w_gettimeofday_r7\n"); }
/* frame walk: guest frames chain through r7 ([r7] = caller's r7, [r7+4] = return address) */
typedef struct { uint32_t a[4]; unsigned n; } Chain;
static Chain g_chains[32];
static void chain_note(uint32_t r7) {
    uint32_t a[4] = {0, 0, 0, 0};
    for (int k = 0; k < 4; k++) {
        if (r7 < 0x81000000u || r7 >= 0xa0000000u || (r7 & 3)) break;
        uint32_t lr = *(volatile uint32_t *)(uintptr_t)(r7 + 4);
        if (lr < 0x98000000u || lr >= 0x98600000u) break;
        a[k] = lr; r7 = *(volatile uint32_t *)(uintptr_t)r7;
    }
    for (int i = 0; i < 32; i++) {
        if (!memcmp(g_chains[i].a, a, sizeof a)) { g_chains[i].n++; return; }
        if (!g_chains[i].n) { memcpy(g_chains[i].a, a, sizeof a); g_chains[i].n = 1; return; }
    }
}
static void chain_dump(void) {
    for (int i = 0; i < 32 && g_chains[i].n; i++) {
        fprintf(stderr, "[clock] chain x%u:", g_chains[i].n);
        for (int k = 0; k < 4 && g_chains[i].a[k]; k++) fprintf(stderr, " %x", g_chains[i].a[k] - 0x98000000u + 0x1000u);
        fprintf(stderr, "\n");
    }
    fflush(stderr);
}
__attribute__((used)) int w_gettimeofday_r7(int32_t *tv, void *tz, uint32_t r7) { g_gtod_count++; ch_note(g_ch_tod, caller_of_frame(r7)); chain_note(r7); if (g_ch_calls + 1 == 20000) chain_dump(); ch_dump();
    struct timeval t; (void)tz;
    int r = gettimeofday(&t, NULL);
    { /* the game's clock: wall time at start + monotonic process time since (gettimeofday itself can step) */
      extern unsigned long long gl_prof_now(void); static long long base_wall, base_proc; static int based;
      long long p = (long long)gl_prof_now();
      { static long long last_raw, last_p; static int nlog; long long raw = (long long)t.tv_sec * 1000000 + t.tv_usec;
        if (last_raw && nlog < 12) { long long dr = raw - last_raw, dp = p - last_p;
            if (dr < 0 || dr - dp > 5000 || dp - dr > 5000) { nlog++; fprintf(stderr, "[clock] gettimeofday stepped: %+lld us while the process timer moved %lld us\n", dr, dp); fflush(stderr); } }
        last_raw = raw; last_p = p; }
      if (!based) { based = 1; base_wall = (long long)t.tv_sec * 1000000 + t.tv_usec; base_proc = p; }
      long long w = base_wall + (p - base_proc); t.tv_sec = (time_t)(w / 1000000); t.tv_usec = (suseconds_t)(w % 1000000); }
    { extern long long gl_game_time(long long real); long long us = gl_game_time((long long)t.tv_sec * 1000000 + t.tv_usec);
      t.tv_sec = (time_t)(us / 1000000); t.tv_usec = (suseconds_t)(us % 1000000); }
    if (tv) { tv[0] = (int32_t)t.tv_sec; tv[1] = (int32_t)t.tv_usec; }
    return r;
}
static int32_t w_time(int32_t *out) { int32_t t = (int32_t)time(NULL); if (out) *out = t; return t; }
static int w_getpid(void) { return 1; }
static unsigned w_sleep(unsigned s) { { static int n; if (n++ < 10) { fprintf(stderr, "[sys] sleep(%u)\n", s); fflush(stderr); } } usleep(s * 1000000u); return 0; }
extern unsigned gl_adjust_sleep(unsigned);
extern void gl_note_sleep(unsigned long long);
__attribute__((used)) int w_usleep_r7(unsigned us, uint32_t r7);
__attribute__((naked, noinline)) static int w_usleep(unsigned us) { __asm__ volatile(".syntax unified\nmov r1, r7\nb w_usleep_r7\n"); }
__attribute__((used)) int w_usleep_r7(unsigned us, uint32_t r7) { ch_note(g_ch_slp, caller_of_frame(r7)); chain_note(r7); g_req_sleep_last = us; g_req_sleep_n++; us = gl_adjust_sleep(us); { static int n; if (n++ < 10) { fprintf(stderr, "[sys] usleep(%u)\n", us); fflush(stderr); } }
    if (!us) return usleep(0); struct timeval a, b; gettimeofday(&a, NULL); int r = usleep(us); gettimeofday(&b, NULL);
    gl_note_sleep((unsigned long long)((b.tv_sec - a.tv_sec) * 1000000LL + (b.tv_usec - a.tv_usec))); return r; }

/* ---- math (softfp wrappers) ---- */
#define F1(n) static float SOFT w_##n(float x) { return n(x); }
F1(sinf) F1(cosf) F1(tanf) F1(asinf) F1(acosf) F1(atanf) F1(floorf) F1(ceilf)
static float  SOFT w_atan2f(float y, float x) { return atan2f(y, x); }
static float  SOFT w_fmodf(float x, float y) { return fmodf(x, y); }
static double SOFT w_fmod(double x, double y) { return fmod(x, y); }
static double SOFT w_pow(double x, double y) { return pow(x, y); }

/* ---- compiler-rt style helpers the guest imports from libgcc ---- */
static long long SOFT w_divdi3(long long a, long long b) { return a / b; }
static long long SOFT w_moddi3(long long a, long long b) { return a % b; }
static unsigned long long SOFT w_udivdi3(unsigned long long a, unsigned long long b) { return a / b; }
static unsigned long long SOFT w_umoddi3(unsigned long long a, unsigned long long b) { return a % b; }
static int SOFT w_divsi3(int a, int b) { return a / b; }
static int SOFT w_modsi3(int a, int b) { return a % b; }
static unsigned SOFT w_udivsi3(unsigned a, unsigned b) { return a / b; }
static unsigned SOFT w_umodsi3(unsigned a, unsigned b) { return a % b; }
static long long SOFT w_fixdfdi(double x) { return (long long)x; }
static long long SOFT w_fixsfdi(float x) { return (long long)x; }
static unsigned long long SOFT w_fixunsdfdi(double x) { return (unsigned long long)x; }
static double SOFT w_floatdidf(long long x) { return (double)x; }
static float  SOFT w_floatdisf(long long x) { return (float)x; }

/* ---- BSD ctype: guest macros read ___DefaultRuneLocale directly ---- */
#define RT_A 0x100u
#define RT_C 0x200u
#define RT_D 0x400u
#define RT_G 0x800u
#define RT_L 0x1000u
#define RT_P 0x2000u
#define RT_S 0x4000u
#define RT_U 0x8000u
#define RT_X 0x10000u
#define RT_B 0x20000u
#define RT_R 0x40000u
#define RT_I 0x80000u
typedef struct {
    char magic[8], encoding[32]; uint32_t sgetrune, sputrune; int32_t invalid_rune;
    uint32_t runetype[256]; int32_t maplower[256], mapupper[256]; uint8_t rest[256];
} rune_locale_t;
static rune_locale_t rune;
static void init_rune(void) {
    memcpy(rune.magic, "RuneMagA", 8); strcpy(rune.encoding, "NONE");
    for (int c = 0; c < 256; c++) {
        uint32_t t = 0;
        if (c < 128) {
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) t |= RT_A | RT_G | RT_R | RT_I;
            if (c >= 'a' && c <= 'z') t |= RT_L;
            if (c >= 'A' && c <= 'Z') t |= RT_U;
            if (c >= '0' && c <= '9') t |= RT_D | RT_G | RT_R | RT_I | (1u << 16 >> 16);
            if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')) t |= RT_X;
            if (c < 32 || c == 127) t |= RT_C;
            if (c == ' ' || (c >= 9 && c <= 13)) t |= RT_S;
            if (c == ' ') t |= RT_B | RT_R | RT_I;
            if (c > 32 && c < 127 && !((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))) t |= RT_P | RT_G | RT_R | RT_I;
        }
        rune.runetype[c] = t;
        rune.maplower[c] = (c >= 'A' && c <= 'Z') ? c + 32 : c;
        rune.mapupper[c] = (c >= 'a' && c <= 'z') ? c - 32 : c;
    }
}
static int w___maskrune(int c, unsigned long mask) { return (c >= 0 && c < 256) ? (int)(rune.runetype[c] & mask) : 0; }
static int w___tolower(int c) { return (c >= 'A' && c <= 'Z') ? c + 32 : c; }


/* ---- NULL-tolerant string functions: nil ObjC strings reach these as NULL; treat them as "" ---- */
#define NS_(p) ((p) ? (p) : "")
static size_t n_strlen(const char *s) { return strlen(NS_(s)); }
static char *n_strcpy(char *d, const char *s) { return strcpy(d, NS_(s)); }
static char *n_strncpy(char *d, const char *s, size_t n) { return strncpy(d, NS_(s), n); }
static char *n_strcat(char *d, const char *s) { return strcat(d, NS_(s)); }
static char *n_strncat(char *d, const char *s, size_t n) { return strncat(d, NS_(s), n); }
static int n_strcmp(const char *a, const char *b) { return strcmp(NS_(a), NS_(b)); }
static int n_strncmp(const char *a, const char *b, size_t n) { return strncmp(NS_(a), NS_(b), n); }
static char *n_strchr(const char *s, int c) { return strchr(NS_(s), c); }
static char *n_strrchr(const char *s, int c) { return strrchr(NS_(s), c); }
static char *n_strstr(const char *a, const char *b) { return strstr(NS_(a), NS_(b)); }
static int n_atoi(const char *s) { return atoi(NS_(s)); }
static int n_strcasecmp(const char *a, const char *b) { return strcasecmp(NS_(a), NS_(b)); }
static int n_strncasecmp(const char *a, const char *b, size_t n) { return strncasecmp(NS_(a), NS_(b), n); }

typedef struct { const char *name; void *fn; } lshim_t;
static const lshim_t table[] = {
    {"_puts", w_puts}, {"_printf", w_printf}, {"_fprintf", w_fprintf}, {"_sprintf", w_sprintf},
    {"___sprintf_chk", w___sprintf_chk}, {"___strcpy_chk", w___strcpy_chk}, {"_sscanf", w_sscanf},
    {"_vprintf", w_vprintf}, {"_vfprintf", w_vfprintf}, {"_vsprintf", w_vsprintf}, {"_vsnprintf", w_vsnprintf},
    {"_vfscanf", w_vfscanf}, {"_fopen", w_fopen},
    {"_fclose", fclose}, {"_fflush", bq_fflush}, {"_fgetc", fgetc}, {"_fgets", fgets}, {"_fputc", fputc},
    {"_fputs", fputs}, {"_fread", fread}, {"_fwrite", fwrite}, {"_fseek", fseek}, {"_ftell", ftell}, {"_ungetc", ungetc},
    {"_strcasecmp", n_strcasecmp}, {"_strncasecmp", n_strncasecmp},
    {"_strlen", n_strlen}, {"_strcpy", n_strcpy}, {"_strncpy", n_strncpy}, {"_strcat", n_strcat}, {"_strncat", n_strncat},
    {"_strcmp", n_strcmp}, {"_strncmp", n_strncmp}, {"_strchr", n_strchr}, {"_strrchr", n_strrchr}, {"_strstr", n_strstr}, {"_atoi", n_atoi}, {"_strcspn", strcspn}, {"_strtok", strtok},
    {"_strtod", w_strtod}, {"_memset_pattern16", w_memset_pattern16},
    {"_gettimeofday", w_gettimeofday}, {"_time", w_time}, {"_getpid", w_getpid}, {"_sleep", w_sleep}, {"_usleep", w_usleep},
    {"_sinf", w_sinf}, {"_cosf", w_cosf}, {"_tanf", w_tanf}, {"_asinf", w_asinf}, {"_acosf", w_acosf}, {"_atanf", w_atanf},
    {"_floorf", w_floorf}, {"_ceilf", w_ceilf}, {"_atan2f", w_atan2f}, {"_fmodf", w_fmodf}, {"_fmod", w_fmod}, {"_pow", w_pow},
    {"___divdi3", w_divdi3}, {"___moddi3", w_moddi3}, {"___udivdi3", w_udivdi3}, {"___umoddi3", w_umoddi3},
    {"___divsi3", w_divsi3}, {"___modsi3", w_modsi3}, {"___udivsi3", w_udivsi3}, {"___umodsi3", w_umodsi3},
    {"___fixdfdi", w_fixdfdi}, {"___fixsfdi", w_fixsfdi}, {"___fixunsdfdi", w_fixunsdfdi},
    {"___floatdidf", w_floatdidf}, {"___floatdisf", w_floatdisf},
    {"___maskrune", w___maskrune}, {"___tolower", w___tolower},
};
extern void *objc_import_lookup(const char *);
extern void *sys_shim_lookup(const char *);
extern void *gl_shim_lookup(const char *);
extern void *objc_native_class_symbol(const char *);
void *libc_shim_lookup(const char *name) {
    { void *o = gl_shim_lookup(name); if (o) return o; }
    { void *o = objc_import_lookup(name); if (o) return o; }
    { void *o = sys_shim_lookup(name); if (o) return o; }
    for (size_t i = 0; i < sizeof table / sizeof *table; i++) if (!strcmp(table[i].name, name)) return table[i].fn;
    { extern void *extra_shim_lookup(const char *); void *o = extra_shim_lookup(name); if (o) return o; }
    return NULL;
}

/* Data symbols: the guest slot must hold the ADDRESS of the variable. */
static FILE *g_stdout_var, *g_stderr_var;
void *libc_data_lookup(const char *name) {
    { void *o = objc_native_class_symbol(name); if (o) return o; }
    if (!strcmp(name, "___stdoutp")) { g_stdout_var = stdout; return &g_stdout_var; }
    if (!strcmp(name, "___stderrp")) { g_stderr_var = stderr; return &g_stderr_var; }
    if (!strcmp(name, "__DefaultRuneLocale")) { init_rune(); return &rune; }
    { extern void *extra_data_lookup(const char *); void *o = extra_data_lookup(name); if (o) return o; }
    return calloc(1, 4096);   /* zeroed placeholder (ObjC classes, block isa, ... filled in later) */
}
