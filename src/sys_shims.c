/* pthread / semaphore / dir / stat / uname shims for the iOS armv7 guest.
 * The guest allocates the iOS structs (pthread_mutex_t = 44 bytes, pthread_attr_t = 40, ...), so the guest
 * storage is treated as opaque: word[0] = signature, word[1] = pointer to the real host object. */
#define _GNU_SOURCE
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <psp2/io/fcntl.h>
#include <psp2/kernel/threadmgr.h>

#ifdef __arm__
#define SOFT __attribute__((pcs("aapcs")))
#else
#define SOFT
#endif

/* tiny global spinlock for lazy-init races */
static volatile int g_lock;
static void glock(void)   { while (__atomic_exchange_n(&g_lock, 1, __ATOMIC_ACQUIRE)) usleep(1); }
static void gunlock(void) { __atomic_store_n(&g_lock, 0, __ATOMIC_RELEASE); }

/* ---------------- mutex ---------------- */
static pthread_mutex_t *hmutex(uint32_t *g) {
    if (!g[1]) {                       /* also covers PTHREAD_MUTEX_INITIALIZER (sig 0x32AAABA7, rest zero) */
        glock();
        if (!g[1]) {
            pthread_mutex_t *m = calloc(1, sizeof *m);
            pthread_mutex_init(m, NULL);
            g[1] = (uint32_t)(uintptr_t)m;
        }
        gunlock();
    }
    return (pthread_mutex_t *)(uintptr_t)g[1];
}
static int w_mutex_init(uint32_t *g, const void *attr) {
    (void)attr; g[1] = 0; g[0] = 0x4D555458u; hmutex(g); return 0;
}
static int w_mutex_destroy(uint32_t *g) {
    if (g[1]) { pthread_mutex_destroy((pthread_mutex_t *)(uintptr_t)g[1]); free((void *)(uintptr_t)g[1]); g[1] = 0; }
    return 0;
}
extern void gl_note_sync(int kind, unsigned long long us); extern unsigned long long gl_prof_now(void);
static int w_mutex_lock(uint32_t *g)   { pthread_mutex_t *m = hmutex(g); if (pthread_mutex_trylock(m) == 0) return 0; unsigned long long t = gl_prof_now(); int r = pthread_mutex_lock(m); gl_note_sync(1, gl_prof_now() - t); return r; }
static int w_mutex_unlock(uint32_t *g) { return pthread_mutex_unlock(hmutex(g)); }


/* ---------------- condition variables (iOS pthread_cond_t = 28 bytes: g[0] sig, g[1] = our host cond) ---------------- */
static pthread_cond_t *hcond(uint32_t *g) {
    if (!g[1]) {                       /* also covers PTHREAD_COND_INITIALIZER (sig 0x3CB0B1BB, rest zero) */
        glock();
        if (!g[1]) { pthread_cond_t *c = calloc(1, sizeof *c); pthread_cond_init(c, NULL); g[1] = (uint32_t)(uintptr_t)c; }
        gunlock();
    }
    return (pthread_cond_t *)(uintptr_t)g[1];
}
static int w_cond_init(uint32_t *g, const void *attr) { (void)attr; g[0] = 0x434F4E44u; g[1] = 0; hcond(g); return 0; }
static int w_cond_destroy(uint32_t *g) { if (g[1]) { pthread_cond_destroy((pthread_cond_t *)(uintptr_t)g[1]); free((void *)(uintptr_t)g[1]); g[1] = 0; } return 0; }
static int w_cond_signal(uint32_t *g) { return pthread_cond_signal(hcond(g)); }
static int w_cond_broadcast(uint32_t *g) { return pthread_cond_broadcast(hcond(g)); }
static int w_cond_wait(uint32_t *g, uint32_t *m) { return pthread_cond_wait(hcond(g), hmutex(m)); }
static int w_cond_timedwait(uint32_t *g, uint32_t *m, const int32_t *ts) {   /* guest timespec: 32-bit tv_sec, tv_nsec (absolute) */
    struct timespec t; t.tv_sec = ts ? (time_t)(uint32_t)ts[0] : 0; t.tv_nsec = ts ? ts[1] : 0;
    int r = pthread_cond_timedwait(hcond(g), hmutex(m), &t);
    return r == ETIMEDOUT ? 60 : r;   /* iOS ETIMEDOUT = 60 */
}
static int w_mutex_trylock(uint32_t *g) { int r = pthread_mutex_trylock(hmutex(g)); return r == 0 ? 0 : 16; /* iOS EBUSY */ }
static int w_setcanceltype(int t, int *old) { (void)t; if (old) *old = 0; return 0; }
static void w_testcancel(void) {}

/* ---------------- attr ---------------- */
static int w_attr_init(uint32_t *a)    { memset(a, 0, 40); a[0] = 0x54484154u; a[1] = 0x40000; return 0; }
static int w_attr_destroy(uint32_t *a) { (void)a; return 0; }
static int w_attr_setstacksize(uint32_t *a, uint32_t sz) { a[1] = sz; return 0; }
static int w_attr_setstackaddr(uint32_t *a, void *p)     { a[2] = (uint32_t)(uintptr_t)p; return 0; }

/* ---------------- threads ---------------- */
typedef struct { void *(*fn)(void *); void *arg; } tstart_t;
/* wait sampler: every 2 ms, look at the engine thread's state; when it is waiting, record (waitType, waitId, the
 * kernel object's name). Shows what the engine blocks on during the slow frames (update167: blocked ~50 ms, not preempted). */
static volatile int g_engine_uid;
volatile unsigned g_ws_frame[4];   /* per-frame wait samples: 0 running, 1 delay (sleep), 2 waitType 0x10, 3 other; read+reset by gl present */
volatile int g_ws_last_id;
typedef struct { unsigned type; int id; unsigned n; char name[32]; } WaitHist;
static void *wait_sampler(void *arg) {
    (void)arg;
    extern int sceKernelChangeThreadCpuAffinityMask(int thid, int mask) __attribute__((weak));
    if (sceKernelChangeThreadCpuAffinityMask) sceKernelChangeThreadCpuAffinityMask(0, 0x10000);
    static WaitHist h[24]; unsigned samples = 0, waiting = 0, running = 0;
    for (;;) {
        usleep(2000);
        if (!g_engine_uid) continue;
        SceKernelThreadInfo ti; memset(&ti, 0, sizeof ti); ti.size = sizeof ti;
        if (sceKernelGetThreadInfo(g_engine_uid, &ti) < 0) continue;
        samples++;
        if (ti.status & SCE_THREAD_WAITING) {
            waiting++; g_ws_frame[ti.waitType == 1 ? 1 : ti.waitType == 0x10 ? 2 : 3]++; if (ti.waitType != 1) g_ws_last_id = ti.waitId;
            int k; for (k = 0; k < 24; k++) { if (h[k].n && h[k].type == ti.waitType && h[k].id == ti.waitId) { h[k].n++; break; } if (!h[k].n) {
                h[k].type = ti.waitType; h[k].id = ti.waitId; h[k].n = 1; h[k].name[0] = 0;
                SceKernelSemaInfo si; memset(&si, 0, sizeof si); si.size = sizeof si;
                if (sceKernelGetSemaInfo(ti.waitId, &si) >= 0) snprintf(h[k].name, sizeof h[k].name, "sema %s", si.name);
                else { SceKernelMutexInfo mi; memset(&mi, 0, sizeof mi); mi.size = sizeof mi;
                       if (sceKernelGetMutexInfo(ti.waitId, &mi) >= 0) snprintf(h[k].name, sizeof h[k].name, "mutex %s", mi.name);
                       else { SceKernelEventFlagInfo ei; memset(&ei, 0, sizeof ei); ei.size = sizeof ei;
                              if (sceKernelGetEventFlagInfo(ti.waitId, &ei) >= 0) snprintf(h[k].name, sizeof h[k].name, "evflag %s", ei.name); } }
                break; } }
        } else { running++; g_ws_frame[0]++; }
        if (samples % 5000 == 0) {   /* every ~10 s */
            fprintf(stderr, "[wait] engine thread over %u samples: running/ready %u, waiting %u:", samples, running, waiting);
            for (int k = 0; k < 24 && h[k].n; k++) fprintf(stderr, " [type %x id %x '%s' x%u]", h[k].type, h[k].id, h[k].name, h[k].n);
            fprintf(stderr, "\n"); fflush(stderr);
            memset(h, 0, sizeof h); samples = waiting = running = 0;
        }
    }
    return NULL;
}
int g_worker_core = 1, g_main_core = 2;   /* vita.cfg workercore= / maincore= (-1 = leave the default). The 'worker' is the game's engine thread: it runs the whole game loop and draws */
static uint32_t vmaddr(const void *p) { uint32_t a = (uint32_t)(uintptr_t)p; return (a >= 0x98000000u && a < 0x98600000u) ? a - 0x98000000u + 0x1000u : a; }
static void *thread_tramp(void *p) {
    tstart_t t = *(tstart_t *)p; free(p);
    /* game worker threads off core 1 (the main/render thread's core): a worker sharing core 1 can steal ~25 ms at a time */
    { extern int sceKernelChangeThreadCpuAffinityMask(int thid, int mask) __attribute__((weak));
      if (sceKernelChangeThreadCpuAffinityMask && g_worker_core >= 0 && g_worker_core <= 2) sceKernelChangeThreadCpuAffinityMask(0, 0x10000 << g_worker_core);
      extern int sceKernelChangeThreadPriority(int thid, int prio) __attribute__((weak));
      if (sceKernelChangeThreadPriority) sceKernelChangeThreadPriority(0, 100);   /* game asks for thread priority 1.0 (highest); 160 is the default, the mixer runs at 65 */ }
    if (!g_engine_uid) { g_engine_uid = sceKernelGetThreadId(); pthread_t st; pthread_attr_t sa; pthread_attr_init(&sa); pthread_attr_setstacksize(&sa, 0x4000); extern int g_verbose; if (g_verbose) pthread_create(&st, &sa, wait_sampler, NULL); }   /* update214: sampler only with verbose=1 */
    fprintf(stderr, "[thread] start fn=vm%x arg=%p (pinned to core %d) thread %p\n", vmaddr(t.fn), t.arg, g_worker_core, (void *)(uintptr_t)pthread_self()); fflush(stderr);
    extern int guest_call2(void *fn, uintptr_t a, uintptr_t b);
    void *r = (void *)(uintptr_t)guest_call2((void *)t.fn, (uintptr_t)t.arg, 0);   /* guest function; saves r9 (scratch on iOS) */
    fprintf(stderr, "[thread] fn=vm%x returned\n", vmaddr(t.fn)); fflush(stderr);
    return r;
}
static int w_pthread_create(uint32_t *out, const uint32_t *attr, void *(*fn)(void *), void *arg) {
    pthread_attr_t a; pthread_attr_init(&a);
    uint32_t stack = (attr && attr[0] == 0x54484154u && attr[1]) ? attr[1] : 0x40000;
    if (stack < 0x20000) stack = 0x20000;
    pthread_attr_setstacksize(&a, stack);
    tstart_t *t = malloc(sizeof *t); t->fn = fn; t->arg = arg;
    fprintf(stderr, "[thread] pthread_create fn=vm%x stack=%x\n", vmaddr(fn), stack); fflush(stderr);
    pthread_t th;
    int r = pthread_create(&th, &a, thread_tramp, t);
    pthread_attr_destroy(&a);
    if (r) { free(t); return r; }
    *out = (uint32_t)(uintptr_t)th;
    return 0;
}
static int w_pthread_join(uint32_t th, void **ret) { fprintf(stderr, "[thread] join %x\n", th); fflush(stderr); return pthread_join((pthread_t)(uintptr_t)th, ret); }
static uint32_t w_pthread_self(void) { return (uint32_t)(uintptr_t)pthread_self(); }
static int w_pthread_equal(uint32_t a, uint32_t b) { return a == b; }
static void w_pthread_exit(void *r) { pthread_exit(r); }
static int w_pthread_cancel(uint32_t th) { (void)th; return 0; }   /* TODO: cooperative cancel if the game relies on it */
extern void gl_note_sync(int kind, unsigned long long us); extern unsigned long long gl_prof_now(void);
static int w_sched_yield(void) { unsigned long long t = gl_prof_now(); usleep(1); gl_note_sync(0, gl_prof_now() - t); return 0; }

/* ---------------- named semaphores (iOS only has sem_open) ---------------- */
typedef struct { pthread_mutex_t m; pthread_cond_t c; int v; } hsem_t;
/* POSIX named semaphores: the same name returns the SAME semaphore (the game opens "alchemyNNN" many times and
 * relies on that; there is no sem_unlink import). The value only applies when the semaphore is created. */
static struct { char name[64]; hsem_t *s; } semtab[64]; static int n_semtab;
volatile unsigned g_sem_wait, g_sem_post, g_sem_try, g_sem_blocked_now; volatile void *g_sem_last_blocked;
static void *w_sem_open(const char *name, int oflag, ...) {
    unsigned value = 0;
    va_list ap; va_start(ap, oflag);
    (void)va_arg(ap, int);             /* mode */
    value = va_arg(ap, unsigned);
    va_end(ap);
    hsem_t *s = NULL; int fresh = 0;
    glock();
    for (int i = 0; name && i < n_semtab; i++) if (!strcmp(semtab[i].name, name)) { s = semtab[i].s; break; }
    if (!s) {
        s = calloc(1, sizeof *s);
        pthread_mutex_init(&s->m, NULL); pthread_cond_init(&s->c, NULL); s->v = (int)value; fresh = 1;
        if (name && n_semtab < 64) { strncpy(semtab[n_semtab].name, name, 63); semtab[n_semtab].s = s; n_semtab++; }
    }
    gunlock();
    { static int n; if (n++ < 40) { fprintf(stderr, "[sem] open %s value=%u -> %p%s\n", name ? name : "?", value, (void *)s, fresh ? " (new)" : " (existing)"); fflush(stderr); } }
    return s;
}
static int w_sem_close(hsem_t *s) { (void)s; return 0; }
static void sem_ev(const char *op, hsem_t *s, void *ra) {
    static volatile int n;
    if (n < 60) { n++; fprintf(stderr, "[sem] %s %p v=%d caller=vm%x tid=%x\n", op, (void *)s, s->v, vmaddr(ra), (unsigned)(uintptr_t)pthread_self()); fflush(stderr); }
}
static int w_sem_post(hsem_t *s) {
    g_sem_post++; sem_ev("post", s, __builtin_return_address(0));
    pthread_mutex_lock(&s->m); s->v++; pthread_cond_broadcast(&s->c); pthread_mutex_unlock(&s->m); return 0;
}
static int w_sem_wait(hsem_t *s) {
    g_sem_wait++; sem_ev("wait", s, __builtin_return_address(0));
    pthread_mutex_lock(&s->m);
    if (s->v <= 0) {
        { static volatile int nb; if (nb < 20) { nb++; fprintf(stderr, "[sem] BLOCKING wait %p v=%d caller=vm%x tid=%x\n", (void *)s, s->v, vmaddr(__builtin_return_address(0)), (unsigned)(uintptr_t)pthread_self()); fflush(stderr); } }
        __atomic_add_fetch(&g_sem_blocked_now, 1, __ATOMIC_RELAXED); g_sem_last_blocked = s;
        { unsigned long long t0 = gl_prof_now(); while (s->v <= 0) pthread_cond_wait(&s->c, &s->m); gl_note_sync(2, gl_prof_now() - t0); }
        __atomic_sub_fetch(&g_sem_blocked_now, 1, __ATOMIC_RELAXED);
    }
    s->v--; pthread_mutex_unlock(&s->m); return 0;
}
static int w_sem_trywait(hsem_t *s) {
    int r = -1; g_sem_try++; sem_ev("trywait", s, __builtin_return_address(0));
    pthread_mutex_lock(&s->m);
    if (s->v > 0) { s->v--; r = 0; } else { errno = EAGAIN; gl_note_sync(3, 0); }
    pthread_mutex_unlock(&s->m); return r;
}
static int w_sem_getvalue(hsem_t *s, int *out) { *out = s->v; return 0; }

/* The game calls exit()/abort() when something is wrong. Real exit() tears the process down under the running threads
 * (=> a confusing crash), so log the caller (guest code address) and park the thread instead. */
static void park_forever(void) { for (;;) usleep(100000); }
static void w_exit(int code) {
    fprintf(stderr, "[exit] guest called exit(%d) from vm%x tid=%x -- parking thread\n", code, vmaddr(__builtin_return_address(0)), (unsigned)(uintptr_t)pthread_self()); fflush(stderr);
    park_forever();
}
static void w_abort(void) {
    fprintf(stderr, "[exit] guest called abort() from vm%x tid=%x -- parking thread\n", vmaddr(__builtin_return_address(0)), (unsigned)(uintptr_t)pthread_self()); fflush(stderr);
    park_forever();
}

/* ---------------- directories (iOS 6 dirent: ino32, reclen16, type8, namlen8, name[256]) ---------------- */
typedef struct { uint32_t ino; uint16_t reclen; uint8_t type, namlen; char name[256]; } g_dirent_t;
static g_dirent_t g_de;
static char dpath[512];
static const char *map_dir(const char *p) {
    if (p && !strcmp(p, "log.txt")) p = "game_log.txt";
    if (!p || strchr(p, ':') || p[0] == '/') return p;
    snprintf(dpath, sizeof dpath, GAME_DIR "%s", p);
    return dpath;
}
static void *w_opendir(const char *p) { return opendir(map_dir(p)); }
static void *w_readdir(void *d) {
    struct dirent *e = readdir((DIR *)d);
    if (!e) return NULL;
    memset(&g_de, 0, sizeof g_de);
    g_de.ino = 1; g_de.type = 0; g_de.namlen = (uint8_t)strlen(e->d_name);
    strncpy(g_de.name, e->d_name, 255); g_de.reclen = sizeof g_de;
    return &g_de;
}
static int w_closedir(void *d) { return closedir((DIR *)d); }

/* ---------------- stat (iOS 6 struct stat, 96 bytes) ---------------- */
static int w_stat(const char *path, uint8_t *g) {
    struct stat st;
    int sr = stat(map_dir(path), &st);
#ifndef NO_FILE_TRACE
    { static int n; if (n++ < 100) { fprintf(stderr, "[stat] %s -> %d\n", path ? path : "(null)", sr); fflush(stderr); } }
#endif
    if (sr != 0) return -1;
    memset(g, 0, 96);
    *(uint32_t *)(g + 4)  = 1;                       /* st_ino */
    *(uint16_t *)(g + 8)  = (uint16_t)st.st_mode;    /* st_mode (same S_IF* values on Darwin) */
    *(uint16_t *)(g + 10) = 1;                       /* st_nlink */
    *(uint32_t *)(g + 32) = (uint32_t)st.st_mtime;   /* st_mtimespec.tv_sec (atime @ 24, mtime @ 32, ctime @ 40) */
    *(uint32_t *)(g + 24) = (uint32_t)st.st_mtime;
    *(uint32_t *)(g + 40) = (uint32_t)st.st_mtime;
    *(int64_t *)(g + 48)  = (int64_t)st.st_size;     /* st_size */
    *(int64_t *)(g + 56)  = (int64_t)((st.st_size + 511) / 512);   /* st_blocks */
    *(int32_t *)(g + 64)  = 4096;                    /* st_blksize */
    return 0;
}

/* ---------------- misc ---------------- */
static int w_uname(char *u) {                         /* struct utsname: 5 x 256 */
    memset(u, 0, 5 * 256);
    strcpy(u, "Darwin"); strcpy(u + 256, "iPhone"); strcpy(u + 512, "12.0.0");
    strcpy(u + 768, "Darwin Kernel Version 12.0.0"); strcpy(u + 1024, "iPhone4,1");
    return 0;
}
/* The engine's allocator (dlmalloc-style) grows its heap with sbrk: back it with one big Vita memblock. */
extern void *platform_alloc_heap(uint32_t size);
static uint8_t *g_brk_base; static uint32_t g_brk_cap, g_brk_cur;
static void *w_sbrk(int inc) {
    glock();
    if (!g_brk_base && !g_brk_cap) {
        static const uint32_t tries[] = {56u<<20, 48u<<20, 40u<<20, 32u<<20, 24u<<20, 16u<<20};
        for (unsigned i = 0; i < sizeof tries / sizeof tries[0]; i++) {
#ifdef __arm__
            void *base = platform_alloc_heap(tries[i]);
#else
            void *base = malloc(tries[i]);
#endif
            if (base) { g_brk_base = base; g_brk_cap = tries[i]; break; }
        }
        if (!g_brk_base) g_brk_cap = 1;
#ifndef NO_SBRK_LOG
        fprintf(stderr, "[sbrk] heap arena %u MB at %p\n", g_brk_cap >> 20, (void *)g_brk_base);
#endif
    }
    void *ret = (void *)-1;
    if (g_brk_base) {
        int64_t n = (int64_t)g_brk_cur + inc;
        if (n >= 0 && n <= (int64_t)g_brk_cap) { ret = g_brk_base + g_brk_cur; g_brk_cur = (uint32_t)n; }
        else errno = ENOMEM;
    } else errno = ENOMEM;
    gunlock();
    return ret;
}
void sys_heap_info(uint8_t **base, uint32_t *used) { *base = g_brk_base; *used = g_brk_cur; }
/* allocate the arena now (before vitaGL grabs the remaining memory) */
void sys_preallocate_heap(void) { (void)w_sbrk(0); }
/* BSD mktemp: every trailing X becomes a random alnum (the game builds unique semaphore names with it; newlib's returned the same name every call) */
static char *w_mktemp(char *t) {
    static uint32_t st; static volatile int lk;
    if (!t) return NULL;
    while (__atomic_exchange_n(&lk, 1, __ATOMIC_ACQUIRE)) usleep(1);
    if (!st) st = 0x9e3779b9u ^ (uint32_t)(uintptr_t)t ^ (uint32_t)time(NULL);
    size_t n = strlen(t);
    while (n && t[n - 1] == 'X') {
        st = st * 1664525u + 1013904223u;
        uint32_t r = (st >> 16) % 62;
        t[--n] = (char)(r < 10 ? '0' + r : r < 36 ? 'a' + (r - 10) : 'A' + (r - 36));
    }
    __atomic_store_n(&lk, 0, __ATOMIC_RELEASE);
    return t;
}


/* ---------------- libstdc++ 4.2 file streams (opaque guest object: [1] = FILE*, [2] = fail flag) ---------------- */
static const char *omode(unsigned m, int out) {
    if (!out) return "rb";
    return (m & 1) ? "ab" : "wb";                /* _Ios_Openmode: app=1 ate=2 binary=4 in=8 out=16 trunc=32 */
}
/* operator!() is called on the basic_ios virtual-base subobject (ifstream + 524 here), not on the stream start where our
 * state lives -> keep a registry of live stream objects and map the subobject pointer back to the nearest one below it. */
static uint32_t *g_streams[128];
static void stream_reg(uint32_t *t) {
    glock();
    int slot = -1;
    for (int i = 0; i < 128; i++) { if (g_streams[i] == t) { slot = i; break; } if (slot < 0 && !g_streams[i]) slot = i; }
    if (slot >= 0) g_streams[slot] = t;
    gunlock();
}
static void stream_unreg(uint32_t *t) { glock(); for (int i = 0; i < 128; i++) if (g_streams[i] == t) g_streams[i] = NULL; gunlock(); }
static void fs_ctor_in(uint32_t *t, const char *path, unsigned mode) {
    stream_reg(t);
    (void)mode; t[2] = 0; t[1] = (uint32_t)(uintptr_t)fopen(map_dir(path), "rb"); if (!t[1]) t[2] = 1;
    if (t[1]) setvbuf((FILE *)(uintptr_t)t[1], NULL, _IOFBF, 64 * 1024);   /* read in 64 KB chunks */
    { static int n; if (n++ < 200) { fprintf(stderr, "[ifstream] %s -> %s\n", path ? path : "(null)", t[1] ? "ok" : "FAIL"); fflush(stderr); } }
}
static void fs_ctor_out(uint32_t *t)  { stream_reg(t); t[1] = 0; t[2] = 0; }
static void fs_open_out(uint32_t *t, const char *path, unsigned mode) {
    stream_reg(t);
    t[2] = 0; t[1] = (uint32_t)(uintptr_t)fopen(map_dir(path), omode(mode, 1)); if (!t[1]) t[2] = 1;
}
static void fs_close(uint32_t *t) { if (t[1]) { fclose((FILE *)(uintptr_t)t[1]); t[1] = 0; } }
static void fs_dtor(uint32_t *t) { fs_close(t); stream_unreg(t); }
static void *fs_read(uint32_t *t, char *buf, int n) {
    FILE *f = (FILE *)(uintptr_t)t[1];
    int ok = f && (int)fread(buf, 1, (size_t)n, f) == n;
    { static int cnt; if (cnt < 40) { cnt++; fprintf(stderr, "[read] caller=vm%x stream=%p n=%d -> %s tell=%ld", vmaddr(__builtin_return_address(0)), (void *)t, n, ok ? "ok" : "SHORT", f ? ftell(f) : -1L);
        if (cnt <= 6 && n >= 16) { fprintf(stderr, " head="); for (int i = 0; i < 32 && i < n; i++) fprintf(stderr, "%02x", (unsigned char)buf[i]); }
        fprintf(stderr, "\n"); fflush(stderr); } }
    if (!ok) t[2] = 1;
    return t;
}
static void *fs_write(uint32_t *t, const char *buf, int n) {
    FILE *f = (FILE *)(uintptr_t)t[1];
    if (!f || (int)fwrite(buf, 1, (size_t)n, f) != n) t[2] = 1;
    return t;
}
/* seekg(fpos<mbstate_t>): on the iOS/APCS ABI a long long inside a struct is only 4-byte aligned, so the 16-byte fpos goes in r1..r3 + stack:
 * r1 = streamoff low word, r2 = streamoff high word (r3 and the stack hold the mbstate_t) */
static void *fs_seekg(uint32_t *t, uint32_t lo, uint32_t hi, uint32_t st0) {
    (void)st0; FILE *f = (FILE *)(uintptr_t)t[1]; if (f) fseek(f, (long)lo, SEEK_SET); t[2] = 0;
    { static int cnt; if (cnt < 40) { cnt++; fprintf(stderr, "[seekg] caller=vm%x stream=%p off=%u hi=%u\n", vmaddr(__builtin_return_address(0)), (void *)t, lo, hi); fflush(stderr); } }
    return t;
}
static int fs_not(uint32_t *ios) {
    uint32_t *best = NULL;
    glock();
    for (int i = 0; i < 128; i++) { uint32_t *t = g_streams[i]; if (t && t < ios && (uintptr_t)ios - (uintptr_t)t < 2048 && (!best || t > best)) best = t; }
    gunlock();
    int fail = (!best || !best[1] || best[2]) ? 1 : 0;
    { static int n; if (n++ < 20) { fprintf(stderr, "[ios] operator! this=%p stream=%p -> %d\n", (void *)ios, (void *)best, fail); fflush(stderr); } }
    return fail;
}

/* ---------------- Blocks runtime helpers (captured ObjC objects are never really refcounted here) ---------------- */
static void blk_noop(void) {}


typedef struct { const char *name; void *fn; } sshim_t;
static const sshim_t stable[] = {
    {"_pthread_mutex_init", w_mutex_init}, {"_pthread_mutex_destroy", w_mutex_destroy},
    {"_pthread_mutex_lock", w_mutex_lock}, {"_pthread_mutex_unlock", w_mutex_unlock}, {"_pthread_mutex_trylock", w_mutex_trylock},
    {"_pthread_cond_init", w_cond_init}, {"_pthread_cond_destroy", w_cond_destroy}, {"_pthread_cond_signal", w_cond_signal},
    {"_pthread_cond_broadcast", w_cond_broadcast}, {"_pthread_cond_wait", w_cond_wait}, {"_pthread_cond_timedwait", w_cond_timedwait},
    {"_pthread_setcanceltype", w_setcanceltype}, {"_pthread_testcancel", w_testcancel},
    {"_pthread_attr_init", w_attr_init}, {"_pthread_attr_destroy", w_attr_destroy},
    {"_pthread_attr_setstacksize", w_attr_setstacksize}, {"_pthread_attr_setstackaddr", w_attr_setstackaddr},
    {"_pthread_create", w_pthread_create}, {"_pthread_join", w_pthread_join}, {"_pthread_self", w_pthread_self},
    {"_pthread_equal", w_pthread_equal}, {"_pthread_exit", w_pthread_exit}, {"_pthread_cancel", w_pthread_cancel},
    {"_sched_yield", w_sched_yield},
    {"_sem_open", w_sem_open}, {"_sem_close", w_sem_close}, {"_sem_post", w_sem_post}, {"_sem_wait", w_sem_wait},
    {"_sem_trywait", w_sem_trywait}, {"_sem_getvalue", w_sem_getvalue},
    {"_opendir", w_opendir}, {"_readdir", w_readdir}, {"_closedir", w_closedir},
    {"__ZNSt14basic_ifstreamIcSt11char_traitsIcEEC1EPKcSt13_Ios_Openmode", fs_ctor_in},
    {"__ZNSt14basic_ifstreamIcSt11char_traitsIcEE5closeEv", fs_close},
    {"__ZNSt14basic_ifstreamIcSt11char_traitsIcEED1Ev", fs_dtor},
    {"__ZNSt14basic_ofstreamIcSt11char_traitsIcEEC1Ev", fs_ctor_out},
    {"__ZNSt14basic_ofstreamIcSt11char_traitsIcEE4openEPKcSt13_Ios_Openmode", fs_open_out},
    {"__ZNSt14basic_ofstreamIcSt11char_traitsIcEE5closeEv", fs_close},
    {"__ZNSt14basic_ofstreamIcSt11char_traitsIcEED1Ev", fs_dtor},
    {"__ZNSi4readEPci", fs_read}, {"__ZNSo5writeEPKci", fs_write}, {"__ZNSi5seekgESt4fposI11__mbstate_tE", fs_seekg},
    {"__ZNKSt9basic_iosIcSt11char_traitsIcEEntEv", fs_not},
    {"__Block_object_assign", blk_noop}, {"__Block_object_dispose", blk_noop},
    {"_exit", w_exit}, {"_abort", w_abort}, {"_stat", w_stat}, {"_uname", w_uname}, {"_sbrk", w_sbrk}, {"_mktemp", w_mktemp},
};
void *sys_shim_lookup(const char *name) {
    for (size_t i = 0; i < sizeof stable / sizeof *stable; i++) if (!strcmp(stable[i].name, name)) return stable[i].fn;
    return NULL;
}

/* ---------------- watchdog: heartbeat in the log, and a deliberate crash (=> core dump with every thread's state) if the
 * main loop stops advancing for ~8 s (a hang leaves no dump otherwise) ---------------- */

extern volatile unsigned g_exec_count, g_present_count;
int g_hangdump;   /* vita.cfg hangdump=1: force a crash dump (every thread's state) when nothing is presented for 10 s */
static void *watchdog(void *arg) {
    (void)arg;
    { extern int sceKernelChangeThreadCpuAffinityMask(int thid, int mask) __attribute__((weak)); if (sceKernelChangeThreadCpuAffinityMask) sceKernelChangeThreadCpuAffinityMask(0, 0x10000); }
    { FILE *cf = fopen(GAME_DIR "vita.cfg", "r"); if (cf) { char line[128]; int v; while (fgets(line, sizeof line, cf)) if (sscanf(line, "hangdump=%d", &v) == 1) g_hangdump = v; fclose(cf); } }
    unsigned lastp = 0; int stalled = 0;
    for (int t = 1; ; t++) {            /* one beat = 0.5 s */
        usleep(500000);
        unsigned p = g_present_count;
        if (t % 4 == 0) { static unsigned heap_used; if (t % 20 == 0 || !heap_used) { struct mallinfo mi = mallinfo(); heap_used = (unsigned)mi.uordblks; }
          fprintf(stderr, "[hb] beat=%d heap_used=%u presents=%u timer_execs=%u sem wait=%u post=%u blocked_now=%u\n", t, heap_used, p, g_exec_count, g_sem_wait, g_sem_post, g_sem_blocked_now);
          fflush(stderr); }
        if (p == lastp) stalled++; else stalled = 0;
        lastp = p;
        if (stalled == 20) { fprintf(stderr, "[hb] nothing presented for 10 s%s\n", g_hangdump ? " -> forcing crash dump" : " (vita.cfg hangdump=1 forces a crash dump)"); fflush(stderr);
            if (g_hangdump) *(volatile int *)0 = 1; }
    }
    return NULL;
}
void sys_start_watchdog(void) {
    pthread_t th; pthread_attr_t a; pthread_attr_init(&a); pthread_attr_setstacksize(&a, 0x10000);
    pthread_create(&th, &a, watchdog, NULL);
}
