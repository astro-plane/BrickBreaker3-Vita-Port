/* Import resolution: real shims where the ABI matches, logging stubs everywhere else. */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "macho.h"
#include "binds_gen.h"
#include <stdint.h>
int missing_import2(int idx, void *ra);
#include "stubs_gen.h"

int g_strict_stubs;   /* vita.cfg strictstubs=1: an unimplemented import aborts (crash dump) instead of returning 0 */
int missing_import2(int idx, void *ra) {
    /* Unimplemented imports log their first few calls (with the guest caller) and return 0, so one run shows many gaps. */
    static unsigned char seen[N_IMPORTS];
    uint32_t a = (uint32_t)(uintptr_t)ra; if (a >= 0x98000000u && a < 0x98600000u) a = a - 0x98000000u + 0x1000u;
    if (seen[idx] < 3) { seen[idx]++; fprintf(stderr, "[stub] UNIMPLEMENTED %s (%s) called from vm%x%s\n", import_names[idx], import_cats[idx], a, seen[idx] == 3 ? " (further calls not logged)" : ""); fflush(stderr); }
    if (g_strict_stubs) abort();
    return 0;
}
/* --- C++ runtime (Apple libstdc++ 4.2 ABI, only what the engine imports) --- */
#include <stdint.h>
#include <pthread.h>
/* qsort with a guest comparator: newlib's qsort may keep values in r9 across compar calls, and r9 is scratch for the
 * guest (iOS ABI), so every compare goes through guest_call2 (saves r9). */
extern int guest_call2(void *fn, uintptr_t a, uintptr_t b);
static void *g_qs_fn; static pthread_mutex_t g_qs_mx = PTHREAD_MUTEX_INITIALIZER;
static int qs_cmp(const void *a, const void *b) { return guest_call2(g_qs_fn, (uintptr_t)a, (uintptr_t)b); }
static void w_qsort(void *base, size_t n, size_t sz, int (*cmp)(const void *, const void *)) {
    pthread_mutex_lock(&g_qs_mx); void *prev = g_qs_fn; g_qs_fn = (void *)cmp; qsort(base, n, sz, qs_cmp); g_qs_fn = prev; pthread_mutex_unlock(&g_qs_mx);
}
static void *shim_new(unsigned n)      { return malloc(n); }
static void  shim_delete(void *p)      { free(p); }
static int   shim_cxa_atexit(void (*f)(void *), void *a, void *d) { (void)f; (void)a; (void)d; return 0; }
static int   shim_guard_acquire(char *g) { if (*g) return 0; return 1; }   /* ARM EABI guard: byte 0 */
static void  shim_guard_release(char *g) { *g = 1; }
static void  shim_noop(void) {}
static void  shim_pure_virtual(void) { abort(); }

typedef struct { const char *name; void *fn; } shim_t;
static const shim_t shims[] = {
    /* libc: plain-pointer/int signatures are ABI-identical */
    {"_malloc", malloc}, {"_free", free}, {"_calloc", calloc}, {"_realloc", realloc},
    {"_memcpy", memcpy}, {"_memmove", memmove}, {"_memset", memset}, {"_memcmp", memcmp},
    {"_strlen", strlen}, {"_strcmp", strcmp}, {"_strncmp", strncmp}, {"_strcpy", strcpy}, {"_strncpy", strncpy},
    {"_strcat", strcat}, {"_strncat", strncat}, {"_strchr", strchr}, {"_strrchr", strrchr}, {"_strstr", strstr},
    {"_atoi", atoi}, {"_qsort", w_qsort}, {"_abort", abort}, {"_exit", exit},
    /* C++ */
    {"__Znwm", shim_new}, {"__Znam", shim_new}, {"__ZdlPv", shim_delete}, {"__ZdaPv", shim_delete},
    {"___cxa_atexit", shim_cxa_atexit}, {"___cxa_guard_acquire", shim_guard_acquire},
    {"___cxa_guard_release", shim_guard_release}, {"___cxa_pure_virtual", shim_pure_virtual},
    {"__ZNSt8ios_base4InitC1Ev", shim_noop}, {"__ZNSt8ios_base4InitD1Ev", shim_noop},
    /* TODO (needs wrappers, see NOTES.md):
     *  - float-arg functions (sinf, cosf, powf, glClearColor, ...): iOS armv7 is softfp
     *  - pthread_*, sem_*: iOS struct sizes differ (pthread_mutex_t = 44 bytes on iOS armv7)
     *  - stdio FILE*: iOS FILE layout differs; ___stdoutp/___stderrp are data symbols
     *  - std::basic_ifstream/ofstream: game allocates the object; treat storage as opaque + FILE*
     *  - all GLES1 calls -> vitaGL (+ OES_matrix_palette emulation)
     */
};

extern void *libc_shim_lookup(const char *);
extern void *libc_data_lookup(const char *);
void *shim_lookup(const char *name) {
    void *l = libc_shim_lookup(name); if (l) return l;
    for (size_t i = 0; i < sizeof shims / sizeof *shims; i++) if (!strcmp(shims[i].name, name)) return shims[i].fn;
    return NULL;
}
void *shim_for_import(int idx) {
    void *f = shim_lookup(import_names[idx]);
    return f ? f : stub_table[idx];
}

/* ---- stack alignment ----
 * iOS armv7 only keeps sp 4-byte aligned at calls; the Vita libraries (newlib, vitaGL, libstdc++) are built for 8 and
 * use NEON loads/stores with :64 alignment on the stack (_fopen_r faulted on `vld1.64 {d16,d17}, [sp:64]`).
 * Every function import therefore goes through align_call: realign sp to 8, copy 16 words of stack arguments, call,
 * restore. align_idx takes the import index in ip (from the per-import entries in align_thunks_gen.h),
 * align_call_ip takes the target itself in ip (objc_msgSend uses it for native methods). */
uint32_t g_imp_tgt[N_IMPORTS]; uint32_t g_imp_cnt[N_IMPORTS];   /* calls per import (profiling: the top ones are logged each second) */
#ifdef __arm__
__asm__(".syntax unified\n.arm\n.text\n.align 2\n"
".global align_idx\n.type align_idx,%function\nalign_idx:\n"
"  push {r4, r5, r6, lr}\n"
"  ldr r4, =g_imp_cnt\n"
"  ldr r5, [r4, ip, lsl #2]\n"
"  add r5, r5, #1\n"
"  str r5, [r4, ip, lsl #2]\n"
"  ldr r4, =g_imp_tgt\n"
"  ldr ip, [r4, ip, lsl #2]\n"
"  b align_body\n"
".global align_call_ip\n.type align_call_ip,%function\nalign_call_ip:\n"
"  push {r4, r5, r6, lr}\n"
"align_body:\n"
"  mov r4, sp\n"
"  sub r5, sp, #64\n"
"  bic r5, r5, #7\n"
"  mov sp, r5\n"
"  ldr r6, [r4, #16]\n  str r6, [sp, #0]\n"
"  ldr r6, [r4, #20]\n  str r6, [sp, #4]\n"
"  ldr r6, [r4, #24]\n  str r6, [sp, #8]\n"
"  ldr r6, [r4, #28]\n  str r6, [sp, #12]\n"
"  ldr r6, [r4, #32]\n  str r6, [sp, #16]\n"
"  ldr r6, [r4, #36]\n  str r6, [sp, #20]\n"
"  ldr r6, [r4, #40]\n  str r6, [sp, #24]\n"
"  ldr r6, [r4, #44]\n  str r6, [sp, #28]\n"
"  ldr r6, [r4, #48]\n  str r6, [sp, #32]\n"
"  ldr r6, [r4, #52]\n  str r6, [sp, #36]\n"
"  ldr r6, [r4, #56]\n  str r6, [sp, #40]\n"
"  ldr r6, [r4, #60]\n  str r6, [sp, #44]\n"
"  ldr r6, [r4, #64]\n  str r6, [sp, #48]\n"
"  ldr r6, [r4, #68]\n  str r6, [sp, #52]\n"
"  ldr r6, [r4, #72]\n  str r6, [sp, #56]\n"
"  ldr r6, [r4, #76]\n  str r6, [sp, #60]\n"
"  blx ip\n"
"  mov sp, r4\n"
"  pop {r4, r5, r6, pc}\n"
".ltorg\n");
#endif
#include "align_thunks_gen.h"
/* Data symbols the import scan lists as functions (C++ vtables/typeinfo/static members, ObjC classes, constants):
 * they must resolve to the object itself, never to a call trampoline. */
static int looks_like_data(const char *n) {
    size_t l = strlen(n);
    if (!strncmp(n, "__ZT", 4)) return n[4] == 'V' || n[4] == 'I' || n[4] == 'T' || n[4] == 'S';   /* vtable/typeinfo/VTT/name (Th/Tv/Tc = thunks: code) */
    if (!strncmp(n, "__ZN", 4)) {                                           /* static data member: nothing after the nested name's closing E */
        const char *p = n + 4; int depth = 0;
        while (*p == 'r' || *p == 'V' || *p == 'K') p++;
        while (*p) {
            if (*p >= '0' && *p <= '9') { size_t k = 0; while (*p >= '0' && *p <= '9') k = k * 10 + (size_t)(*p++ - '0'); if (strlen(p) < k) return 0; p += k; }
            else if (*p == 'S') { if ((p[1] >= '0' && p[1] <= '9') || (p[1] >= 'A' && p[1] <= 'Z') || p[1] == '_') { p = strchr(p + 1, '_'); if (!p) return 0; p++; } else p += 2; }
            else if (*p == 'I') { depth++; p++; }
            else if (*p == 'E') { p++; if (depth == 0) return *p == 0; depth--; }
            else if ((*p == 'C' || *p == 'D') && p[1] >= '0' && p[1] <= '9') return 0;   /* constructor / destructor */
            else p++;
        }
        return 0;
    }
    if (!strncmp(n, "__ZSt", 5)) {                                          /* std::<name> with no parameters, e.g. std::nothrow */
        const char *p = n + 5; size_t k = 0; while (*p >= '0' && *p <= '9') k = k * 10 + (size_t)(*p++ - '0');
        if (k && strlen(p) == k) return 1;
    }
    if (!strncmp(n, "_OBJC_", 6) || !strncmp(n, "___std", 6) || (n[0] == '_' && n[1] == 'k' && n[2] >= 'A' && n[2] <= 'Z')) return 1;
    return 0;
}
/* Hot leaf routines that never touch the stack with alignment hints: called directly, no trampoline. memcpy alone
 * ran 5.6 million times a second while menus load (the game's image reader copies a few bytes at a time). */
static const char *const g_direct[] = {
    "_memcpy", "_memmove", "_memset", "_memcmp", "_bzero", "_strlen", "_strcmp", "_strncmp", "_strcpy", "_strncpy", "_strchr",
    "___divsi3", "___udivsi3", "___modsi3", "___umodsi3", "___divdi3", "___udivdi3", "___moddi3", "___umoddi3",
    "__Unwind_SjLj_Register", "__Unwind_SjLj_Unregister", NULL };
static int no_align(const char *n) {   /* these must see the guest's own stack frame / registers */
    if (looks_like_data(n)) return 1;
    for (int i = 0; g_direct[i]; i++) if (!strcmp(n, g_direct[i])) return 1;
    return !strcmp(n, "_setjmp") || !strcmp(n, "_longjmp") || !strcmp(n, "_objc_msgSend") || !strcmp(n, "_objc_msgSend_stret")
        || !strcmp(n, "_objc_msgSendSuper2") || !strcmp(n, "_objc_msgSendSuper") || !strcmp(n, "_objc_msgSend_fpret");
}
uint32_t shim_resolve(int idx) {
    if (import_is_data[idx]) return (uint32_t)(uintptr_t)libc_data_lookup(import_names[idx]);
    void *f = shim_for_import(idx);
#ifdef __arm__
    if (f && !no_align(import_names[idx])) { g_imp_tgt[idx] = (uint32_t)(uintptr_t)f; return (uint32_t)(uintptr_t)imp_thunks[idx]; }
#endif
    return (uint32_t)(uintptr_t)f;
}

/* once a second (from the GL present path): the imports called most often in the last second */
void imp_prof_report(void) {
    static uint32_t prev[N_IMPORTS];
    int top[8] = {-1,-1,-1,-1,-1,-1,-1,-1}; uint32_t topn[8] = {0};
    uint32_t total = 0;
    for (int i = 0; i < N_IMPORTS; i++) {
        uint32_t d = g_imp_cnt[i] - prev[i]; prev[i] = g_imp_cnt[i]; total += d;
        for (int k = 0; k < 8; k++) if (d > topn[k]) { for (int j = 7; j > k; j--) { topn[j] = topn[j - 1]; top[j] = top[j - 1]; } topn[k] = d; top[k] = i; break; }
    }
    if (total < 20000) return;   /* only busy seconds */
    fprintf(stderr, "[imports] %u calls/s:", total);
    for (int k = 0; k < 8 && top[k] >= 0; k++) fprintf(stderr, " %s=%u", import_names[top[k]], topn[k]);
    fprintf(stderr, "\n");
}
