/* extra_shims.c - imports Baroque never needed, added for Block Breaker 3 (but nothing here is BB3-specific):
 *   C++ runtime: std::string / streams / rb-tree from the VitaSDK libstdc++ (old COW ABI, see tools/cxx_map.py),
 *                exception entry points (a throw is logged and crashes on purpose: SjLj unwinding is not implemented),
 *   setjmp/longjmp on the guest's jmp_buf, double math, wide strings, time, misc libc,
 *   sockets/AddressBook/SystemConfiguration (offline: everything fails politely),
 *   CoreFoundation basics, CGRect helpers, AudioSession + RemoteIO AudioUnit (render callback -> sceAudioOut).
 * Guest ABI: iOS armv7 = softfp -> anything with float/double arguments or results is SOFT. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <math.h>
#include <time.h>
#include <errno.h>
#include <wchar.h>
#include <unistd.h>
#include <pthread.h>
#include <psp2/audioout.h>
#include <psp2/kernel/processmgr.h>

#ifdef __arm__
#define SOFT __attribute__((pcs("aapcs")))
#else
#define SOFT
#endif
static uint32_t vm_of(void *p) { uint32_t a = (uint32_t)(uintptr_t)p; return (a >= 0x98000000u && a < 0x98600000u) ? a - 0x98000000u + 0x1000u : a; }
#define CALLER vm_of(__builtin_return_address(0))
static void die_here(void) { fflush(stderr); *(volatile int *)0 = 0xdead; }

/* ======================= C++ ======================= */
#include "cxx_map_gen.h"
extern void *h_ostream_insert_double(void *os, double d) __asm__("_ZNSo9_M_insertIdEERSoT_");
static void *SOFT w_ostream_insert_double(void *os, double d) { return h_ostream_insert_double(os, d); }

/* ostream << unsigned long: the VitaSDK libstdc++ does not export _M_insert<unsigned long>, so format it here and
 * write it with __ostream_insert (stream flags such as hex/width are ignored - only used for logging text) */
extern void *h_ostream_insert_chars(void *os, const char *s, int n) __asm__("_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_i");
static void *w_ostream_insert_ulong(void *os, unsigned long v) { char b[16]; int n = snprintf(b, sizeof b, "%lu", v); return h_ostream_insert_chars(os, b, n); }
static void *w_cxa_allocate_exception(unsigned n) { char *p = calloc(1, n + 128); return p ? p + 128 : NULL; }
static void w_cxa_free_exception(void *p) { if (p) free((char *)p - 128); }
static void w_cxa_throw(void *obj, uint32_t *tinfo, void *dtor) {
    const char *nm = tinfo ? (const char *)(uintptr_t)tinfo[1] : "?";
    fprintf(stderr, "[c++] the game threw an exception (type %s) from vm%x - exceptions are not supported yet, crashing on purpose\n", nm ? nm : "?", CALLER);
    (void)obj; (void)dtor; die_here();
}
static void w_cxa_rethrow(void) { fprintf(stderr, "[c++] __cxa_rethrow from vm%x\n", CALLER); die_here(); }
static void w_cxa_call_unexpected(void *e) { (void)e; fprintf(stderr, "[c++] __cxa_call_unexpected from vm%x\n", CALLER); die_here(); }
static void w_terminate(void) { fprintf(stderr, "[c++] std::terminate from vm%x\n", CALLER); die_here(); }
static void w_throw_msg(const char *m) { fprintf(stderr, "[c++] std::__throw_*(\"%s\") from vm%x\n", m ? m : "", CALLER); die_here(); }
static void w_throw_bad_alloc(void) { fprintf(stderr, "[c++] std::bad_alloc (out of memory) from vm%x\n", CALLER); die_here(); }
static void *w_cxa_begin_catch(void *p) { return p; }
static void w_noop(void) {}
static int w_personality(void) { fprintf(stderr, "[c++] personality routine called (unwinding) from vm%x\n", CALLER); die_here(); return 0; }
/* SjLj: every function with cleanups registers a context on entry and unregisters it on exit. The list is only walked
 * when unwinding, which never happens here (a throw crashes first), so both are no-ops. */
static void w_sjlj_register(void *fc) { (void)fc; }
static void w_sjlj_unregister(void *fc) { (void)fc; }
static void w_sjlj_resume(void *e) { (void)e; fprintf(stderr, "[c++] _Unwind_SjLj_Resume from vm%x\n", CALLER); die_here(); }

/* objc @synchronized / properties / @try */
static pthread_mutex_t g_sync; static pthread_once_t g_sync_once = PTHREAD_ONCE_INIT;
static void sync_init(void) { pthread_mutexattr_t a; pthread_mutexattr_init(&a); pthread_mutexattr_settype(&a, PTHREAD_MUTEX_RECURSIVE); pthread_mutex_init(&g_sync, &a); }
static int w_objc_sync_enter(void *o) { (void)o; pthread_once(&g_sync_once, sync_init); pthread_mutex_lock(&g_sync); return 0; }
static int w_objc_sync_exit(void *o) { (void)o; pthread_mutex_unlock(&g_sync); return 0; }
static void *w_objc_getProperty(void *self, void *cmd, int off, int atomic) { (void)cmd; (void)atomic; return self ? *(void **)((char *)self + off) : NULL; }
static void w_objc_copyStruct(void *d, const void *s, int n, int atomic, int strong) { (void)atomic; (void)strong; memcpy(d, s, (size_t)n); }

/* ======================= setjmp / longjmp =======================
 * The guest's jmp_buf is 27 ints. Saved: r4-r11, sp, lr (10 words) + d8-d15 (16 words) = 26 words. r9 is scratch for the
 * guest, nothing of ours runs between the guest's setjmp and longjmp. */
__attribute__((naked, noinline)) static int w_setjmp(void *buf) {
    __asm__ volatile(".syntax unified\n"
        "stmia r0!, {r4-r11}\n"
        "mov r1, sp\n"
        "str r1, [r0], #4\n"
        "str lr, [r0], #4\n"
        "vstmia r0, {d8-d15}\n"
        "mov r0, #0\n"
        "bx lr\n");
}
__attribute__((naked, noinline)) static void w_longjmp(void *buf, int val) {
    __asm__ volatile(".syntax unified\n"
        "ldmia r0!, {r4-r11}\n"
        "ldr r2, [r0], #4\n"
        "ldr lr, [r0], #4\n"
        "vldmia r0, {d8-d15}\n"
        "mov sp, r2\n"
        "movs r0, r1\n"
        "it eq\n"
        "moveq r0, #1\n"
        "bx lr\n");
}

/* ======================= math / libc ======================= */
#define D1(n) static double SOFT w_##n(double x) { return n(x); }
D1(acos) D1(asin) D1(atan) D1(ceil) D1(cos) D1(exp) D1(floor) D1(log) D1(log10) D1(sin) D1(tan)
static double SOFT w_atan2(double y, double x) { return atan2(y, x); }
static double SOFT w_ldexp(double x, int e) { return ldexp(x, e); }
static float SOFT w_logf(float x) { return logf(x); }
static double SOFT w_atof(const char *s) { return atof(s); }
static double SOFT w_floatundidf(unsigned long long x) { return (double)x; }
static int *w___error(void) { return &errno; }
static void *w___memcpy_chk(void *d, const void *s, size_t n, size_t dn) { (void)dn; return memcpy(d, s, n); }
static void w___assert_rtn(const char *fn, const char *file, int line, const char *e) {
    fprintf(stderr, "[assert] %s:%d %s: %s (vm%x)\n", file ? file : "?", line, fn ? fn : "?", e ? e : "?", CALLER); die_here();
}
static int w_snprintf(char *s, size_t n, const char *f, ...) { va_list a; va_start(a, f); int r = vsnprintf(s, n, f, a); va_end(a); return r; }
static int w_swprintf(wchar_t *s, size_t n, const wchar_t *f, ...) { va_list a; va_start(a, f); int r = vswprintf(s, n, f, a); va_end(a); return r; }
static size_t w_strlcpy(char *d, const char *s, size_t n) { size_t l = strlen(s); if (n) { size_t k = l < n - 1 ? l : n - 1; memcpy(d, s, k); d[k] = 0; } return l; }
static char g_mpath[512];
static const char *mpath(const char *p) { if (!p || strchr(p, ':') || p[0] == '/') return p; snprintf(g_mpath, sizeof g_mpath, GAME_DIR "%s", p); return g_mpath; }
static int w_remove(const char *p) { return remove(mpath(p)); }
static int w_chdir(const char *p) { (void)p; return 0; }
static char *w_getcwd(char *b, size_t n) { static char d[] = GAME_DIR_NS; if (!b) return strdup(d); if (n <= strlen(d)) return NULL; strcpy(b, d); return b; }
static char *w_realpath(const char *p, char *out) { const char *m = mpath(p); if (!out) return strdup(m); strcpy(out, m); return out; }
static char *w_getenv(const char *n) { (void)n; return NULL; }
static int w_system(const char *c) { (void)c; return -1; }
static int w_sigaction(int s, const void *a, void *o) { (void)s; (void)a; if (o) memset(o, 0, 12); return 0; }
static int w_sigprocmask(int h, const void *s, void *o) { (void)h; (void)s; if (o) memset(o, 0, 4); return 0; }
static int w_fcntl(int fd, int cmd, ...) { (void)fd; (void)cmd; return 0; }
static int w_ioctl(int fd, unsigned long r, ...) { (void)fd; (void)r; errno = EINVAL; return -1; }
static void *w_dlsym(void *h, const char *n) { fprintf(stderr, "[dlsym] %s -> NULL\n", n ? n : "?"); (void)h; return NULL; }
static int w_sysctlbyname(const char *name, void *oldp, size_t *oldlen, void *newp, size_t newlen) {
    (void)newp; (void)newlen;
    const char *v = NULL;
    if (name && !strcmp(name, "hw.machine")) v = "iPhone3,1";            /* iPhone 4 */
    else if (name && !strcmp(name, "hw.model")) v = "N90AP";
    fprintf(stderr, "[sysctl] %s -> %s\n", name ? name : "?", v ? v : "(unsupported)");
    if (!v) { errno = ENOENT; return -1; }
    size_t l = strlen(v) + 1;
    if (oldp && oldlen && *oldlen >= l) memcpy(oldp, v, l);
    if (oldlen) *oldlen = l;
    return 0;
}
/* time: guest time_t is 32-bit; guest struct tm = 9 ints + long tm_gmtoff + char *tm_zone */
typedef struct { int v[9]; int32_t gmtoff; const char *zone; } gtm_t;
static void tm_out(const struct tm *t, gtm_t *g) { memset(g, 0, sizeof *g); if (!t) return; int src[9] = { t->tm_sec, t->tm_min, t->tm_hour, t->tm_mday, t->tm_mon, t->tm_year, t->tm_wday, t->tm_yday, t->tm_isdst }; memcpy(g->v, src, sizeof src); g->zone = "UTC"; }
static gtm_t g_tm;
static gtm_t *w_localtime(const int32_t *t) { time_t h = t ? (time_t)*t : 0; struct tm r; localtime_r(&h, &r); tm_out(&r, &g_tm); return &g_tm; }
static gtm_t *w_localtime_r(const int32_t *t, gtm_t *out) { time_t h = t ? (time_t)*t : 0; struct tm r; localtime_r(&h, &r); tm_out(&r, out); return out; }
static char *w_ctime(const int32_t *t) { time_t h = t ? (time_t)*t : 0; return ctime(&h); }
static int w_nanosleep(const int32_t *ts, int32_t *rem) { if (rem) rem[0] = rem[1] = 0; if (ts) usleep((useconds_t)ts[0] * 1000000u + (useconds_t)(ts[1] / 1000)); return 0; }
static uint64_t w_mach_absolute_time(void) { return (uint64_t)sceKernelGetProcessTimeWide() * 1000ull; }   /* ns, timebase 1/1 */
static int w_mach_timebase_info(uint32_t *tb) { tb[0] = 1; tb[1] = 1; return 0; }
static unsigned char *w_CC_MD5(const void *d, uint32_t n, unsigned char *md) { (void)d; (void)n; memset(md, 0, 16); return md; }   /* only used for online ids */
/* aio: done synchronously. iOS armv7 struct aiocb: int fildes @0, off_t offset @8 (64-bit), void *buf @16, size_t nbytes @20 */
static struct { void *cb; int ret, err; } g_aio[32];
static int w_aio_read(uint8_t *cb) {
    int fd = *(int *)cb; int64_t off; memcpy(&off, cb + 8, 8); void *buf = *(void **)(cb + 16); size_t n = *(uint32_t *)(cb + 20);
    int r = -1, e = 0;
    if (lseek(fd, (off_t)off, SEEK_SET) >= 0) r = (int)read(fd, buf, n);
    if (r < 0) e = errno ? errno : EIO;
    int k = 0; for (int i = 0; i < 32; i++) if (g_aio[i].cb == cb || !g_aio[i].cb) { k = i; break; }
    g_aio[k].cb = cb; g_aio[k].ret = r; g_aio[k].err = e;
    { static int nl; if (nl++ < 20) fprintf(stderr, "[aio] read fd=%d off=%lld n=%u -> %d\n", fd, (long long)off, (unsigned)n, r); }
    return 0;
}
static int w_aio_error(void *cb) { for (int i = 0; i < 32; i++) if (g_aio[i].cb == cb) return g_aio[i].err; return 0; }
static int w_aio_return(void *cb) { for (int i = 0; i < 32; i++) if (g_aio[i].cb == cb) { g_aio[i].cb = NULL; return g_aio[i].ret; } return -1; }

/* ======================= network: offline ======================= */
static int w_net_fail(void) { errno = ENETDOWN; return -1; }
static void *w_null(void) { return NULL; }
static int w_zero(void) { return 0; }
static int w_gethostname(char *b, size_t n) { if (n) { strncpy(b, "vita", n); b[n - 1] = 0; } return 0; }
static uint32_t w_inet_addr(const char *s) { (void)s; return 0xffffffffu; }
static const char *w_inet_ntoa(uint32_t a) { (void)a; return "0.0.0.0"; }
static int w_select(int n, void *r, void *w, void *e, int32_t *tv) { (void)n; (void)r; (void)w; (void)e; if (tv) usleep((useconds_t)tv[0] * 1000000u + (useconds_t)tv[1]); return 0; }

/* ======================= CoreFoundation / CoreGraphics bits ======================= */
extern void *objc_nsstr(const char *utf8);
extern const char *objc_nsstr_utf8(void *s);
static uint32_t g_cf_dummy[16];
static void *w_CFBundleGetMainBundle(void) { return g_cf_dummy; }
static void *w_CFBundleCopyBundleURL(void *b) { (void)b; return g_cf_dummy + 4; }
static void *w_CFURLCopyFileSystemPath(void *u, int style) { (void)u; (void)style; return objc_nsstr(GAME_DIR_NS); }
static int w_CFStringGetCString(void *s, char *buf, int n, unsigned enc) {
    (void)enc; const char *u = s ? objc_nsstr_utf8(s) : NULL; if (!u || n <= 0) return 0;
    if ((int)strlen(u) >= n) return 0; strcpy(buf, u); return 1;
}
static void *w_CFURLCreateStringByAddingPercentEscapes(void *a, void *s, void *l, void *r, unsigned e) { (void)a; (void)l; (void)r; (void)e; return s; }
static int SOFT w_CFRunLoopRunInMode(void *mode, double secs, int ret) { (void)mode; (void)ret; if (secs > 0 && secs < 1) usleep((useconds_t)(secs * 1e6)); return 3; /* kCFRunLoopRunTimedOut */ }
static void *w_CFArrayCreateMutable(void *a, int n, void *cb) { (void)a; (void)n; (void)cb; return calloc(1, 64); }
typedef struct { float x, y, w, h; } CGRect_;
static float SOFT w_CGRectGetWidth(CGRect_ r) { return r.w; }
static float SOFT w_CGRectGetHeight(CGRect_ r) { return r.h; }
static float SOFT w_CGRectGetMinX(CGRect_ r) { return r.x; }
static float SOFT w_CGRectGetMinY(CGRect_ r) { return r.y; }
static int SOFT w_CGRectIsEmpty(CGRect_ r) { return r.w <= 0.f || r.h <= 0.f; }
static CGRect_ SOFT w_CGRectInset(CGRect_ r, float dx, float dy) { r.x += dx; r.y += dy; r.w -= 2 * dx; r.h -= 2 * dy; return r; }
static CGRect_ SOFT w_CGRectOffset(CGRect_ r, float dx, float dy) { r.x += dx; r.y += dy; return r; }
static const float g_cg_identity[6] = { 1, 0, 0, 1, 0, 0 };
static const float g_cg_zero[4];

/* ======================= audio: AudioSession + RemoteIO AudioUnit =======================
 * The game sets a render callback and a stream format on its RemoteIO unit and starts it; a thread here then pulls
 * GRAIN frames at a time from the guest callback and queues them on a BGM port at the game's sample rate. */
typedef struct { double rate; uint32_t fmt, flags, bpp, fpp, bpf, ch, bits, res; } ASBD;
static struct { void *proc, *refcon; ASBD f; int have_f, running; pthread_t th; } g_au;
static int g_au_nosound;
__attribute__((naked, noinline)) static int gcall6(void *fn, uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e, uint32_t f) {
    __asm__ volatile(".syntax unified\n"
        "push {r4-r7, r9, lr}\n"       /* 24 bytes: the caller's d, e, f are at sp+24.. */
        "mov r4, r0\n"
        "mov r0, r1\n"
        "mov r1, r2\n"
        "mov r2, r3\n"
        "ldr r3, [sp, #24]\n"
        "ldr r5, [sp, #28]\n"
        "ldr r6, [sp, #32]\n"
        "sub sp, sp, #8\n"
        "str r5, [sp]\n"
        "str r6, [sp, #4]\n"
        "blx r4\n"
        "add sp, sp, #8\n"
        "pop {r4-r7, r9, pc}\n");
}
static int au_rate_ok(int r) { static const int ok[] = { 8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000 }; for (unsigned i = 0; i < sizeof ok / sizeof *ok; i++) if (ok[i] == r) return 1; return 0; }
#define AU_GRAIN 1024
static void *au_thread(void *arg) {
    (void)arg;
    extern int sceKernelChangeThreadPriority(int thid, int prio) __attribute__((weak));
    if (sceKernelChangeThreadPriority) sceKernelChangeThreadPriority(0, 65);
    int rate = g_au.have_f ? (int)g_au.f.rate : 44100, ch = g_au.have_f ? (int)g_au.f.ch : 2, bits = g_au.have_f ? (int)g_au.f.bits : 16;
    int isfloat = g_au.have_f && (g_au.f.flags & 1), nonint = g_au.have_f && (g_au.f.flags & 0x20);
    if (!au_rate_ok(rate)) rate = 44100;
    int port = sceAudioOutOpenPort(SCE_AUDIO_OUT_PORT_TYPE_BGM, AU_GRAIN, rate, SCE_AUDIO_OUT_MODE_STEREO);
    fprintf(stderr, "[audiounit] output thread: %d Hz, %d ch, %d bit%s%s -> port %d\n", rate, ch, bits, isfloat ? " float" : "", nonint ? " non-interleaved" : "", port); fflush(stderr);
    if (port < 0) return NULL;
    { int v[2] = { SCE_AUDIO_VOLUME_0DB, SCE_AUDIO_VOLUME_0DB }; sceAudioOutSetVolume(port, SCE_AUDIO_VOLUME_FLAG_L_CH | SCE_AUDIO_VOLUME_FLAG_R_CH, v); }
    static int16_t out[2][AU_GRAIN * 2]; int cur = 0;
    uint8_t *gbuf = calloc(1, AU_GRAIN * 8 * 2);
    uint32_t abl[1 + 3 * 2]; uint32_t flags = 0; double ts[8]; memset(ts, 0, sizeof ts);
    unsigned calls = 0;
    while (g_au.running) {
        int bpf = (bits / 8) * (nonint ? 1 : ch);
        abl[0] = nonint ? (uint32_t)ch : 1;
        for (int b = 0; b < (int)abl[0]; b++) { abl[1 + b * 3] = nonint ? 1 : (uint32_t)ch; abl[2 + b * 3] = (uint32_t)(AU_GRAIN * bpf); abl[3 + b * 3] = (uint32_t)(uintptr_t)(gbuf + b * AU_GRAIN * bpf); }
        flags = 0; ts[0] += AU_GRAIN; ((uint32_t *)ts)[14] = 1;   /* mSampleTime, mFlags = kAudioTimeStampSampleTimeValid */
        gcall6(g_au.proc, (uint32_t)(uintptr_t)g_au.refcon, (uint32_t)(uintptr_t)&flags, (uint32_t)(uintptr_t)ts, 0, AU_GRAIN, (uint32_t)(uintptr_t)abl);
        if (calls++ == 0) { fprintf(stderr, "[audiounit] first render callback returned\n"); fflush(stderr); }
        int16_t *o = out[cur];
        for (int i = 0; i < AU_GRAIN; i++) {
            int l, r;
            if (isfloat) {
                const float *s = (const float *)gbuf;
                float fl = nonint ? s[i] : s[i * ch], fr = ch > 1 ? (nonint ? ((const float *)(gbuf + AU_GRAIN * 4))[i] : s[i * ch + 1]) : fl;
                l = (int)(fl * 32767.f); r = (int)(fr * 32767.f);
            } else if (bits == 8) {
                const uint8_t *s = gbuf; l = ((int)s[i * ch] - 128) << 8; r = ch > 1 ? ((int)s[i * ch + 1] - 128) << 8 : l;
            } else if (bits == 32) {   /* 8.24 fixed (canonical AudioUnit format) */
                const int32_t *s = (const int32_t *)gbuf;
                int32_t a = nonint ? s[i] : s[i * ch], b = ch > 1 ? (nonint ? ((const int32_t *)(gbuf + AU_GRAIN * 4))[i] : s[i * ch + 1]) : a;
                l = a >> 9; r = b >> 9;
            } else {
                const int16_t *s = (const int16_t *)gbuf;
                l = nonint ? s[i] : s[i * ch]; r = ch > 1 ? (nonint ? ((const int16_t *)(gbuf + AU_GRAIN * 2))[i] : s[i * ch + 1]) : l;
            }
            if (l > 32767) l = 32767; if (l < -32768) l = -32768; if (r > 32767) r = 32767; if (r < -32768) r = -32768;
            o[i * 2] = (int16_t)l; o[i * 2 + 1] = (int16_t)r;
        }
        sceAudioOutOutput(port, o);
        cur ^= 1;
    }
    sceAudioOutOutput(port, NULL);
    sceAudioOutReleasePort(port);
    free(gbuf);
    return NULL;
}
static int w_AudioSessionInitialize(void *rl, void *mode, void *cb, void *ud) { (void)rl; (void)mode; (void)cb; (void)ud; return 0; }
static int w_AudioSessionSetActive(int a) { (void)a; return 0; }
static int w_AudioSessionSetProperty(uint32_t id, uint32_t n, const void *d) { (void)n; (void)d; fprintf(stderr, "[audiosession] set '%c%c%c%c'\n", id >> 24, id >> 16, id >> 8, id); return 0; }
static int w_AudioSessionGetProperty(uint32_t id, uint32_t *n, void *d) {
    if (!n || !d) return 0;
    memset(d, 0, *n);
    if (id == 0x63687372u && *n >= 8) { double r = 44100.0; memcpy(d, &r, 8); }          /* 'chsr' hardware sample rate */
    else if (id == 0x6F757463u && *n >= 4) *(uint32_t *)d = 2;                            /* 'outc' output channels */
    return 0;
}
static uint32_t g_au_comp[4], g_au_unit[4];
static void *w_AudioComponentFindNext(void *c, void *desc) { (void)c; (void)desc; return g_au_comp; }
static int w_AudioComponentInstanceNew(void *c, void **out) { (void)c; if (out) *out = g_au_unit; return 0; }
static int w_AudioComponentInstanceDispose(void *u) { (void)u; return 0; }
static int w_AudioUnitInitialize(void *u) { (void)u; return 0; }
static int w_AudioUnitUninitialize(void *u) { (void)u; return 0; }
static int w_AudioUnitSetProperty(void *u, uint32_t id, uint32_t scope, uint32_t el, const void *d, uint32_t n) {
    (void)u;
    if (id == 23 && d && n >= 8) { g_au.proc = ((void *const *)d)[0]; g_au.refcon = ((void *const *)d)[1]; fprintf(stderr, "[audiounit] render callback vm%x\n", vm_of(g_au.proc)); }
    else if (id == 8 && d && n >= sizeof(ASBD)) {
        memcpy(&g_au.f, d, sizeof(ASBD)); g_au.have_f = 1;
        fprintf(stderr, "[audiounit] stream format scope %u el %u: %.0f Hz fmt %08x flags %x bytes/frame %u ch %u bits %u\n", scope, el, g_au.f.rate, g_au.f.fmt, g_au.f.flags, g_au.f.bpf, g_au.f.ch, g_au.f.bits);
    } else fprintf(stderr, "[audiounit] set property %u scope %u el %u (%u bytes)\n", id, scope, el, n);
    fflush(stderr);
    return 0;
}
static int w_AudioOutputUnitStart(void *u) {
    (void)u;
    if (g_au_nosound) { fprintf(stderr, "[audiounit] start ignored (nosound)\n"); return 0; }
    if (!g_au.proc || g_au.running) return 0;
    g_au.running = 1;
    pthread_attr_t a; pthread_attr_init(&a); pthread_attr_setstacksize(&a, 0x10000);
    pthread_create(&g_au.th, &a, au_thread, NULL);
    return 0;
}
static int w_AudioOutputUnitStop(void *u) { (void)u; if (g_au.running) { g_au.running = 0; pthread_join(g_au.th, NULL); } return 0; }

/* ======================= tables ======================= */
typedef struct { const char *name; void *fn; } xs_t;
static const xs_t xtab[] = {
    {"__ZNSo9_M_insertIdEERSoT_", w_ostream_insert_double}, {"__ZNSo9_M_insertImEERSoT_", w_ostream_insert_ulong},
    {"___cxa_allocate_exception", w_cxa_allocate_exception}, {"___cxa_free_exception", w_cxa_free_exception},
    {"___cxa_throw", w_cxa_throw}, {"___cxa_rethrow", w_cxa_rethrow}, {"___cxa_call_unexpected", w_cxa_call_unexpected},
    {"___cxa_begin_catch", w_cxa_begin_catch}, {"___cxa_end_catch", w_noop}, {"___cxa_guard_abort", w_noop},
    {"__ZSt9terminatev", w_terminate}, {"__ZSt17__throw_bad_allocv", w_throw_bad_alloc},
    {"__ZSt19__throw_logic_errorPKc", w_throw_msg}, {"__ZSt20__throw_length_errorPKc", w_throw_msg}, {"__ZSt20__throw_out_of_rangePKc", w_throw_msg},
    {"___gxx_personality_sj0", w_personality}, {"___objc_personality_v0", w_personality},
    {"__Unwind_SjLj_Register", w_sjlj_register}, {"__Unwind_SjLj_Unregister", w_sjlj_unregister}, {"__Unwind_SjLj_Resume", w_sjlj_resume},
    {"_objc_begin_catch", w_cxa_begin_catch}, {"_objc_end_catch", w_noop},
    {"_objc_sync_enter", w_objc_sync_enter}, {"_objc_sync_exit", w_objc_sync_exit},
    {"_objc_getProperty", w_objc_getProperty}, {"_objc_copyStruct", w_objc_copyStruct},
    {"_setjmp", w_setjmp}, {"_longjmp", w_longjmp},
    {"_acos", w_acos}, {"_asin", w_asin}, {"_atan", w_atan}, {"_atan2", w_atan2}, {"_ceil", w_ceil}, {"_cos", w_cos},
    {"_exp", w_exp}, {"_floor", w_floor}, {"_ldexp", w_ldexp}, {"_log", w_log}, {"_log10", w_log10}, {"_sin", w_sin},
    {"_tan", w_tan}, {"_logf", w_logf}, {"_atof", w_atof}, {"___floatundidf", w_floatundidf},
    {"_atol", atol}, {"_strtol", strtol}, {"_memchr", memchr}, {"_strpbrk", strpbrk}, {"_strlcpy", w_strlcpy},
    {"_strerror", strerror}, {"_rand", rand}, {"_srand", srand}, {"_putchar", putchar}, {"_perror", perror},
    {"_snprintf", w_snprintf}, {"_swprintf", w_swprintf}, {"___error", w___error}, {"___memcpy_chk", w___memcpy_chk},
    {"___assert_rtn", w___assert_rtn}, {"_remove", w_remove}, {"_chdir", w_chdir}, {"_getcwd", w_getcwd},
    {"_realpath$DARWIN_EXTSN", w_realpath}, {"_getenv", w_getenv}, {"_system", w_system},
    {"_sigaction", w_sigaction}, {"_sigprocmask", w_sigprocmask}, {"_fcntl", w_fcntl}, {"_ioctl", w_ioctl},
    {"_fileno", fileno}, {"_close", close}, {"_dlsym", w_dlsym}, {"_sysctlbyname", w_sysctlbyname},
    {"_localtime", w_localtime}, {"_localtime_r", w_localtime_r}, {"_ctime", w_ctime}, {"_nanosleep", w_nanosleep},
    {"_mach_absolute_time", w_mach_absolute_time}, {"_mach_timebase_info", w_mach_timebase_info}, {"_CC_MD5", w_CC_MD5},
    {"_aio_read", w_aio_read}, {"_aio_error", w_aio_error}, {"_aio_return", w_aio_return},
    {"_wcscat", wcscat}, {"_wcscmp", wcscmp}, {"_wcscpy", wcscpy}, {"_wcslen", wcslen}, {"_wcstombs", wcstombs},
    {"_mbstowcs", mbstowcs}, {"_wmemchr", wmemchr}, {"_wmemcmp", wmemcmp}, {"_wmemcpy", wmemcpy}, {"_wmemmove", wmemmove},
    {"_socket", w_net_fail}, {"_connect", w_net_fail}, {"_bind", w_net_fail}, {"_listen", w_net_fail}, {"_accept", w_net_fail},
    {"_send", w_net_fail}, {"_sendto", w_net_fail}, {"_recv", w_net_fail}, {"_recvfrom", w_net_fail},
    {"_setsockopt", w_net_fail}, {"_getsockopt", w_net_fail}, {"_getsockname", w_net_fail},
    {"_gethostbyname", w_null}, {"_gethostname", w_gethostname}, {"_inet_addr", w_inet_addr}, {"_inet_ntoa", w_inet_ntoa},
    {"_select", w_select},
    {"_CFBundleGetMainBundle", w_CFBundleGetMainBundle}, {"_CFBundleCopyBundleURL", w_CFBundleCopyBundleURL},
    {"_CFURLCopyFileSystemPath", w_CFURLCopyFileSystemPath}, {"_CFStringGetCString", w_CFStringGetCString},
    {"_CFURLCreateStringByAddingPercentEscapes", w_CFURLCreateStringByAddingPercentEscapes},
    {"_CFRunLoopRunInMode", w_CFRunLoopRunInMode}, {"_CFArrayCreateMutable", w_CFArrayCreateMutable},
    {"_CFRelease", w_noop}, {"_CFStringAppendCharacters", w_noop},
    {"_CGRectGetWidth", w_CGRectGetWidth}, {"_CGRectGetHeight", w_CGRectGetHeight}, {"_CGRectGetMinX", w_CGRectGetMinX},
    {"_CGRectGetMinY", w_CGRectGetMinY}, {"_CGRectIsEmpty", w_CGRectIsEmpty}, {"_CGRectInset", w_CGRectInset}, {"_CGRectOffset", w_CGRectOffset},
    {"_AudioSessionInitialize", w_AudioSessionInitialize}, {"_AudioSessionSetActive", w_AudioSessionSetActive},
    {"_AudioSessionSetProperty", w_AudioSessionSetProperty}, {"_AudioSessionGetProperty", w_AudioSessionGetProperty},
    {"_AudioComponentFindNext", w_AudioComponentFindNext}, {"_AudioComponentInstanceNew", w_AudioComponentInstanceNew},
    {"_AudioComponentInstanceDispose", w_AudioComponentInstanceDispose}, {"_AudioUnitInitialize", w_AudioUnitInitialize},
    {"_AudioUnitUninitialize", w_AudioUnitUninitialize}, {"_AudioUnitSetProperty", w_AudioUnitSetProperty},
    {"_AudioOutputUnitStart", w_AudioOutputUnitStart}, {"_AudioOutputUnitStop", w_AudioOutputUnitStop},
    {"_ABAddressBookCreate", w_null}, {"_ABAddressBookCopyArrayOfAllPeople", w_null},
    {"_SCNetworkReachabilityCreateWithName", w_null}, {"_SCNetworkReachabilityCreateWithAddress", w_null},
    {"_SCNetworkReachabilityGetFlags", w_zero}, {"_SCNetworkReachabilitySetCallback", w_zero},
    {"_SCNetworkReachabilityScheduleWithRunLoop", w_zero}, {"_SCNetworkReachabilityUnscheduleFromRunLoop", w_zero},
};
void *extra_shim_lookup(const char *name) {
    for (size_t i = 0; i < sizeof xtab / sizeof *xtab; i++) if (!strcmp(xtab[i].name, name)) return xtab[i].fn;
    for (size_t i = 0; i < sizeof cxx_map / sizeof *cxx_map; i++) if (!strcmp(cxx_map[i].guest, name)) return cxx_map[i].host;
    return NULL;
}
void *extra_data_lookup(const char *name) {
    for (size_t i = 0; i < sizeof cxx_map / sizeof *cxx_map; i++) if (!strcmp(cxx_map[i].guest, name)) return cxx_map[i].host;
    if (!strcmp(name, "_CGAffineTransformIdentity")) return (void *)g_cg_identity;
    if (!strcmp(name, "_CGRectZero")) return (void *)g_cg_zero;
    return NULL;
}
void extra_cfg_line(const char *line) { int v; extern int g_strict_stubs; if (sscanf(line, "nosound=%d", &v) == 1) g_au_nosound = v; else if (sscanf(line, "strictstubs=%d", &v) == 1) g_strict_stubs = v; else if (sscanf(line, "barcache=%d", &v) == 1) { extern int g_barcache; g_barcache = v; } }
