/* Minimal Objective-C runtime + tiny Foundation/UIKit stand-ins for the iOS guest.
 * Guest pointers are 32-bit. Everything the guest can point at (selectors, class blobs, objects)
 * comes from objc_g_alloc so it is 32-bit addressable (always true on Vita, via arena on the host). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#include "objc_rt.h"
#include "sjis.h"

typedef uint32_t u32;
typedef void *id;
typedef const char *SEL;

#define PTR(u) ((void *)(uintptr_t)(u))
#define U32(p) ((u32)(uintptr_t)(p))

#ifdef __arm__
#define SOFT __attribute__((pcs("aapcs")))
void *objc_g_alloc(unsigned size) { return calloc(1, size); }
#else
#define SOFT
#include <sys/mman.h>
void *objc_g_alloc(unsigned size) {
    static uint8_t *arena; static size_t used, cap = 64u << 20;
    if (!arena) arena = mmap(NULL, cap, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_32BIT, -1, 0);
    size = (size + 15) & ~15u; void *p = arena + used; used += size; return p;
}
#endif

/* ------------------------------------------------------------------ selectors */
#define SEL_TAB 8192
static SEL sel_tab[SEL_TAB];
static u32 hstr(const char *s) { u32 h = 2166136261u; while (*s) h = (h ^ (uint8_t)*s++) * 16777619u; return h; }
/* the engine thread and the main thread both message objects: serialize runtime tables with one spinlock */
static volatile int rt_lk;
static void rt_lock(void)   { while (__atomic_exchange_n(&rt_lk, 1, __ATOMIC_ACQUIRE)) usleep(1); }
static void rt_unlock(void) { __atomic_store_n(&rt_lk, 0, __ATOMIC_RELEASE); }
static SEL sel_registerName_u(const char *name);
volatile int g_rt_running;   /* set once the run loop starts: from then on log every selector registration (crash hunt) */
SEL sel_registerName(const char *name) {
    if (g_rt_running) {
        static volatile int n;
        if (n < 40) { n++; fprintf(stderr, "[sel] register name=%p caller=%p\n", (const void *)name, __builtin_return_address(0)); fflush(stderr); }
    }
    if ((uintptr_t)name < 0x10000) { fprintf(stderr, "[sel] BAD NAME %p caller=%p\n", (const void *)name, __builtin_return_address(0)); fflush(stderr); name = "(null)"; }
    rt_lock(); SEL r = sel_registerName_u(name); rt_unlock(); return r;
}
static SEL sel_registerName_u(const char *name) {
    u32 i = hstr(name) & (SEL_TAB - 1);
    while (sel_tab[i]) { if (!strcmp(sel_tab[i], name)) return sel_tab[i]; i = (i + 1) & (SEL_TAB - 1); }
    char *c = objc_g_alloc(strlen(name) + 1);
    if (!c) { fprintf(stderr, "[sel] OUT OF HOST MEMORY registering '%s'\n", name); fflush(stderr); return (SEL)"(oom)"; }
    strcpy(c, name); sel_tab[i] = c; return c;
}

/* ------------------------------------------------------------------ class records */
typedef struct { SEL sel; void *imp; } Meth;
typedef struct RClass {
    u32 ptr, super_ptr, inst_size; char *name; struct RClass *meta; int is_meta, native;
    Meth *m; int nm, cap;
} RClass;
#define RC_TAB 2048
static RClass *rc_tab[RC_TAB];
static RClass *rc_list[1024]; static int rc_n;
static RClass *rc_find(u32 ptr) {
    if (!ptr) return NULL;
    u32 i = (ptr >> 2) * 2654435761u >> 21 & (RC_TAB - 1);
    while (rc_tab[i]) { if (rc_tab[i]->ptr == ptr) return rc_tab[i]; i = (i + 1) & (RC_TAB - 1); }
    return NULL;
}
static RClass *rc_new(u32 ptr, const char *name) {
    RClass *k = calloc(1, sizeof *k); k->ptr = ptr; k->name = strdup(name);
    u32 i = (ptr >> 2) * 2654435761u >> 21 & (RC_TAB - 1);
    while (rc_tab[i]) i = (i + 1) & (RC_TAB - 1);
    rc_tab[i] = k; rc_list[rc_n++] = k; return k;
}
static void rc_add(RClass *k, SEL s, void *imp) {
    if (k->nm == k->cap) { k->cap = k->cap ? k->cap * 2 : 8; k->m = realloc(k->m, k->cap * sizeof(Meth)); }
    k->m[k->nm].sel = s; k->m[k->nm].imp = imp; k->nm++;
}
static RClass *class_named(const char *name) {
    for (int i = 0; i < rc_n; i++) if (!rc_list[i]->is_meta && !strcmp(rc_list[i]->name, name)) return rc_list[i];
    return NULL;
}
int objc_class_count(void) { return rc_n; }
void objc_dump_classes(void) {
    for (int i = 0; i < rc_n; i++) fprintf(stderr, "  %s%s ptr=%08x super=%08x size=%u methods=%d%s\n", rc_list[i]->is_meta ? "meta " : "", rc_list[i]->name,
                                           rc_list[i]->ptr, rc_list[i]->super_ptr, rc_list[i]->inst_size, rc_list[i]->nm, rc_list[i]->native ? " (native)" : "");
}

/* native class: class_t-shaped blob (isa, super, cache, vtable, data) + a metaclass blob */
static RClass *native_class(const char *name, const char *super) {
    RClass *k = class_named(name); if (k && k->native) return k;
    u32 *cb = objc_g_alloc(20), *mb = objc_g_alloc(20);
    if (!super && strcmp(name, "NSObject")) super = "NSObject";
    RClass *sk = super ? class_named(super) : NULL;
    if (super && !sk) sk = native_class(super, strcmp(super, "NSObject") ? "NSObject" : NULL);
    k = rc_new(U32(cb), name); k->native = 1; k->inst_size = 32;
    RClass *mk = rc_new(U32(mb), name); mk->native = 1; mk->is_meta = 1; k->meta = mk; mk->meta = mk;
    cb[0] = U32(mb); mb[0] = U32(mb);
    if (sk) { cb[1] = sk->ptr; k->super_ptr = sk->ptr; mb[1] = sk->meta->ptr; mk->super_ptr = sk->meta->ptr; }
    else { mk->super_ptr = k->ptr; mb[1] = U32(cb); }       /* root metaclass inherits from root class */
    return k;
}
static void nm_inst(const char *cls, const char *sel, void *imp) { rc_add(native_class(cls, NULL), sel_registerName(sel), imp); }
static void nm_cls(const char *cls, const char *sel, void *imp) { rc_add(native_class(cls, NULL)->meta, sel_registerName(sel), imp); }

/* ------------------------------------------------------------------ guest metadata */
static macho_t *IMG; static u32 FINAL;
static int in_image(u32 a) { return a >= FINAL && a < FINAL + IMG->vm_size; }
static u32 *gp(u32 a) { return (u32 *)(IMG->base + (a - FINAL)); }
static const char *gcstr(u32 a) { return in_image(a) ? (const char *)(IMG->base + (a - FINAL)) : (const char *)PTR(a); }

static void load_methods(RClass *k, u32 mlist) {
    if (!mlist) return;
    u32 *ml = gp(mlist); u32 cnt = ml[1], ent = ml[0] & ~3u; if (!ent) ent = 12;
    for (u32 i = 0; i < cnt; i++) {
        u32 *m = (u32 *)((uint8_t *)(ml + 2) + i * ent);
        SEL s = sel_registerName(gcstr(m[0])); m[0] = U32(s);       /* uniquify in place */
        rc_add(k, s, PTR(m[2]));
    }
}
static RClass *load_class(u32 cptr) {
    u32 *c = gp(cptr); u32 *ro = gp(c[4] & ~3u);
    RClass *k = rc_new(cptr, gcstr(ro[4]));
    k->super_ptr = c[1]; k->inst_size = ro[2]; k->is_meta = (ro[0] & 1);
    load_methods(k, ro[5]);
    return k;
}
void objc_rt_init(macho_t *m) {
    IMG = m; FINAL = m->final_base ? m->final_base : (u32)(uintptr_t)m->base;
    for (u32 i = 0; i < m->classlist_count; i++) {
        u32 cptr = macho_ptr(m, m->classlist_addr)[i];
        RClass *k = load_class(cptr);
        u32 mptr = gp(cptr)[0];
        if (in_image(mptr)) { k->meta = load_class(mptr); k->meta->meta = k->meta; }
    }
    for (u32 i = 0; i < m->selrefs_count; i++) { u32 *slot = macho_ptr(m, m->selrefs_addr) + i; *slot = U32(sel_registerName(gcstr(*slot))); }
}

/* ------------------------------------------------------------------ dispatch */
static void *nil_imp_fn(void) { return 0; }
#define CACHE 4096
static struct { u32 cls; SEL sel; void *imp; } cache[CACHE];
static struct { u32 cls; SEL sel; } missing[1024]; static int n_missing;
static void *rt_lookup_u(u32 cls, SEL sel);
volatile unsigned g_msg_count; const char *g_msg_ring[8];
void *rt_lookup(u32 cls, SEL sel) { g_msg_ring[g_msg_count & 7] = sel; g_msg_count++; rt_lock(); void *r = rt_lookup_u(cls, sel); rt_unlock(); return r; }
static void *rt_lookup_u(u32 cls, SEL sel) {
#ifndef NO_CALL_TRACE
    { static volatile int n_w;       /* trace every dispatch (all threads) after startup; skip the noisy ones */
      if (n_w < 900 && strcmp(sel, "alloc") && strcmp(sel, "init") && strcmp(sel, "release") && strcmp(sel, "autorelease") && strcmp(sel, "retain") && strcmp(sel, "execute")) {
          n_w++; RClass *r0 = rc_find(cls); fprintf(stderr, "[w] %c[%s %s]\n", (r0 && r0->is_meta) ? '+' : '-', r0 ? r0->name : "?", sel);
      } }
#endif
    u32 h = ((cls >> 2) ^ ((u32)(uintptr_t)sel >> 2) * 31u) & (CACHE - 1);
    if (cache[h].cls == cls && cache[h].sel == sel) return cache[h].imp;
    for (RClass *c = rc_find(cls); c; c = rc_find(c->super_ptr)) {
        for (int i = c->nm - 1; i >= 0; i--) if (c->m[i].sel == sel) {
            cache[h].cls = cls; cache[h].sel = sel; cache[h].imp = c->m[i].imp;
            { static int n_tr; RClass *rc0 = rc_find(cls); if (n_tr < 900 && rc0 && strcmp(sel, "alloc") && strcmp(sel, "init") && strcmp(sel, "release") && strcmp(sel, "autorelease") && strcmp(sel, "retain")) { n_tr++; fprintf(stderr, "[call]%s %c[%s %s]\n", c->native ? "n" : "", rc0->is_meta ? '+' : '-', rc0->name, sel); } }
            return c->m[i].imp;
        }
        if (c->super_ptr == c->ptr) break;
    }
    for (int i = 0; i < n_missing; i++) if (missing[i].cls == cls && missing[i].sel == sel) return (void *)nil_imp_fn;
    if (n_missing < 1024) { missing[n_missing].cls = cls; missing[n_missing].sel = sel; n_missing++; }
    RClass *c = rc_find(cls);
    fprintf(stderr, "[objc] unhandled %c[%s %s] -> nil\n", (c && c->is_meta) ? '+' : '-', c ? c->name : "?", sel); fflush(stderr);
    return (void *)nil_imp_fn;
}
#ifdef __arm__
__asm__(
".syntax unified\n.arm\n.text\n.align 4\n"
".global rt_msgSend\n.type rt_msgSend,%function\nrt_msgSend:\n"
"  cmp r0,#0\n  moveq r1,#0\n  bxeq lr\n"
"  push {r0-r4,lr}\n  mov r4,sp\n  bic r12,r4,#7\n  mov sp,r12\n  ldr r0,[r0]\n  bl rt_lookup\n  mov sp,r4\n  mov r12,r0\n  pop {r0-r4,lr}\n  b rt_go\n"
".global rt_msgSend_stret\n.type rt_msgSend_stret,%function\nrt_msgSend_stret:\n"
"  cmp r1,#0\n  bxeq lr\n"
"  push {r0-r4,lr}\n  mov r4,sp\n  bic r12,r4,#7\n  mov sp,r12\n  ldr r0,[r1]\n  mov r1,r2\n  bl rt_lookup\n  mov sp,r4\n  mov r12,r0\n  pop {r0-r4,lr}\n  b rt_go\n"
".global rt_msgSendSuper2\n.type rt_msgSendSuper2,%function\nrt_msgSendSuper2:\n"
"  push {r0-r4,lr}\n  mov r4,sp\n  bic r12,r4,#7\n  mov sp,r12\n  ldr r0,[r0,#4]\n  ldr r0,[r0,#4]\n  bl rt_lookup\n  mov sp,r4\n  mov r12,r0\n  pop {r0-r4,lr}\n  ldr r0,[r0]\n  b rt_go\n"
/* guest methods (inside the image 0x98000000-0x9a000000): jump straight in; native ones: through the stack aligner */
"rt_go:\n  cmp r12,#0x98000000\n  blo 1f\n  cmp r12,#0x9a000000\n  bxlo r12\n1:\n  b align_call_ip\n"
);
extern void rt_msgSend(void), rt_msgSend_stret(void), rt_msgSendSuper2(void);
#else
static void rt_msgSend(void) {} static void rt_msgSend_stret(void) {} static void rt_msgSendSuper2(void) {}
#endif

/* native -> any method */
/* Calls into guest code must save r9: on iOS armv7 r9 is a scratch register the guest may clobber, while our (AAPCS)
 * code keeps values in it across calls. update126 crashed in the touch loop because the tslot pointer lived in r9
 * across touchesBegan:. */
#ifdef __arm__
__attribute__((naked, noinline)) static id call_guest5(id o, SEL s, id a, id b, id c, void *fn) {
    __asm__ volatile(".syntax unified\n"
        "push {r4, r5, r9, lr}\n"
        "ldr r4, [sp, #16]\n"      /* c  */
        "ldr r5, [sp, #20]\n"      /* fn */
        "sub sp, sp, #8\n"
        "str r4, [sp]\n"
        "blx r5\n"
        "add sp, sp, #8\n"
        "pop {r4, r5, r9, pc}\n");
}
#else
static id call_guest5(id o, SEL s, id a, id b, id c, void *fn) { return ((id (*)(id, SEL, id, id, id))fn)(o, s, a, b, c); }
#endif
static id msg(id o, const char *sel, id a, id b, id c) {
    if (!o) return 0;
    SEL s = sel_registerName(sel);
    void *imp = rt_lookup(*(u32 *)o, s);
    return call_guest5(o, s, a, b, c, imp);
}

/* ------------------------------------------------------------------ native object state (side table) */
typedef struct St { id obj; void *a, *b, *c; double d; int i; struct St *next; } St;
static St *st_tab[1024];
static St *state(id o) {
    u32 h = ((u32)(uintptr_t)o >> 3) & 1023;
    for (St *s = st_tab[h]; s; s = s->next) if (s->obj == o) return s;
    St *s = calloc(1, sizeof *s); s->obj = o; s->next = st_tab[h]; st_tab[h] = s; return s;
}
static id new_obj(u32 cls) {
    RClass *k = rc_find(cls); u32 sz = k && k->inst_size >= 4 ? k->inst_size : 32;
    u32 *o = objc_g_alloc(sz < 32 ? 32 : sz); o[0] = cls;
    /* C++ ivars: like the real runtime, run each class's own .cxx_construct, root class first */
    static SEL s_cxx; if (!s_cxx) s_cxx = sel_registerName(".cxx_construct");
    RClass *chain[24]; int n = 0;
    for (RClass *c = k; c && n < 24; c = rc_find(c->super_ptr)) { chain[n++] = c; if (c->super_ptr == c->ptr) break; }
    for (int i = n - 1; i >= 0; i--) {
        if (chain[i]->native) continue;
        for (int j = 0; j < chain[i]->nm; j++) if (chain[i]->m[j].sel == s_cxx) {
            static int nl; if (nl < 20) { nl++; fprintf(stderr, "[objc] .cxx_construct for %s\n", chain[i]->name); }
            call_guest5((id)o, s_cxx, 0, 0, 0, chain[i]->m[j].imp); break;
        }
    }
    return o;
}

/* ------------------------------------------------------------------ NSObject & friends */
static id i_self(id s, SEL c) { return s; }
static id i_zero(id s, SEL c) { return 0; }
static id i_one(id s, SEL c) { return (id)1; }
static id c_alloc(id cls, SEL c) { return new_obj(U32(cls)); }
static id c_allocWithZone(id cls, SEL c, id z) { return new_obj(U32(cls)); }
static id c_class(id cls, SEL c) { return cls; }
static id i_class(id o, SEL c) { return PTR(*(u32 *)o); }
static id c_new(id cls, SEL c) { return msg(msg(cls, "alloc", 0, 0, 0), "init", 0, 0, 0); }
static id i_isKindOfClass(id o, SEL c, id k) {
    for (RClass *r = rc_find(*(u32 *)o); r; r = rc_find(r->super_ptr)) { if (r->ptr == U32(k)) return (id)1; if (r->super_ptr == r->ptr) break; }
    return 0;
}
static id i_respondsToSelector(id o, SEL c, SEL s) {
    for (RClass *r = rc_find(*(u32 *)o); r; r = rc_find(r->super_ptr)) { for (int i = 0; i < r->nm; i++) if (r->m[i].sel == s) return (id)1; if (r->super_ptr == r->ptr) break; }
    return 0;
}
static id i_isEqual(id o, SEL c, id p) { return (id)(uintptr_t)(o == p); }
static id i_hash(id o, SEL c) { return o; }

/* NSString: CFString layout {isa, flags, chars, length}; dynamic strings reuse it */
static id cfstring_class_ptr;
static id nsstr_new(const char *utf8) {
    u32 *s = objc_g_alloc(16); s[0] = U32(cfstring_class_ptr); s[1] = 0x7c8; char *c = objc_g_alloc(strlen(utf8) + 1); strcpy(c, utf8); s[2] = U32(c); s[3] = (u32)strlen(utf8); return s;
}
static const char *nsstr_utf8(id s) { if (!s) return ""; u32 *w = s; return (w[1] & 0xff) == 0xc8 || (w[1] & 0xff) == 0xd0 ? (const char *)PTR(w[2]) : (const char *)PTR(w[2]); }
static id c_stringWithUTF8String(id cls, SEL c, const char *p) { return nsstr_new(p ? p : ""); }
static id i_UTF8String(id s, SEL c) { return (id)nsstr_utf8(s); }
static id i_length(id s, SEL c) { return (id)(uintptr_t)((u32 *)s)[3]; }
static id c_stringWithString(id cls, SEL c, id s) { return nsstr_new(nsstr_utf8(s)); }


/* ---- NSData + string encoding conversion (the game converts strings through NSData when building its text) ---- */
static id nsdata_new(const void *p, u32 n) {
    id o = new_obj(U32(native_class("NSData", NULL)->ptr)); void *b = objc_g_alloc(n ? n : 1); if (p && n) memcpy(b, p, n);
    state(o)->a = b; state(o)->i = (int)n; return o;
}
static unsigned u8_next(const unsigned char **pp) {            /* decode one UTF-8 code point, advancing */
    const unsigned char *p = *pp; unsigned c = *p++;
    if (c < 0x80) { *pp = p; return c; }
    int n = c >= 0xF0 ? 3 : c >= 0xE0 ? 2 : c >= 0xC0 ? 1 : 0; c &= n == 3 ? 7 : n == 2 ? 15 : 31;
    while (n-- > 0 && (*p & 0xC0) == 0x80) c = (c << 6) | (*p++ & 63);
    *pp = p; return c;
}
static int u8_put(char *o, unsigned c) {
    if (c < 0x80) { o[0] = (char)c; return 1; }
    if (c < 0x800) { o[0] = (char)(0xC0 | (c >> 6)); o[1] = (char)(0x80 | (c & 63)); return 2; }
    if (c < 0x10000) { o[0] = (char)(0xE0 | (c >> 12)); o[1] = (char)(0x80 | ((c >> 6) & 63)); o[2] = (char)(0x80 | (c & 63)); return 3; }
    o[0] = (char)(0xF0 | (c >> 18)); o[1] = (char)(0x80 | ((c >> 12) & 63)); o[2] = (char)(0x80 | ((c >> 6) & 63)); o[3] = (char)(0x80 | (c & 63)); return 4;
}
static int enc_unit(unsigned enc) {   /* 1 = byte encodings, 2 = UTF-16, 4 = UTF-32 */
    switch (enc) { case 10: case 0x94000100: case 0x90000100: return 2; case 0x8c000100: case 0x9c000100: case 0x98000100: return 4; default: return 1; }
}
static int enc_be(unsigned enc) { return enc == 0x90000100 || enc == 0x98000100; }
static int enc_bom(unsigned enc) { return enc == 10 || enc == 0x8c000100; }
static id i_dataUsingEncoding(id s, SEL c, unsigned enc, id lossy) {
    const unsigned char *u = (const unsigned char *)nsstr_utf8(s), *p = u; size_t cap = strlen((const char *)u) * 4 + 8; unsigned char *o = malloc(cap); size_t n = 0;
    int unit = enc_unit(enc), be = enc_be(enc);
    if (enc == 8) { n = sjis_from_utf8((const char *)u, o); unit = 0; }
    if (unit > 1 && enc_bom(enc)) { unsigned bom = 0xFEFF; for (int i = 0; i < unit; i++) o[n++] = (unsigned char)(unit == 2 ? ((bom >> (be ? (1 - i) : i) * 8) & 255) : ((bom >> (be ? (3 - i) : i) * 8) & 255)); }
    while (unit && *p) {
        unsigned cp = u8_next(&p);
        if (unit == 1) {
            if (enc == 4) { n += (size_t)u8_put((char *)o + n, cp); }
            else o[n++] = (unsigned char)((enc == 1 ? cp < 128 : cp < 256) ? cp : '?');
        } else if (unit == 2) {
            unsigned w[2], k = 1; if (cp >= 0x10000) { cp -= 0x10000; w[0] = 0xD800 | (cp >> 10); w[1] = 0xDC00 | (cp & 0x3FF); k = 2; } else w[0] = cp;
            for (unsigned j = 0; j < k; j++) { if (be) { o[n++] = (unsigned char)(w[j] >> 8); o[n++] = (unsigned char)w[j]; } else { o[n++] = (unsigned char)w[j]; o[n++] = (unsigned char)(w[j] >> 8); } }
        } else { for (int i = 0; i < 4; i++) o[n++] = (unsigned char)(cp >> (be ? (3 - i) : i) * 8); }
    }
    id d = nsdata_new(o, (u32)n); free(o);
    { static int k; if (k++ < 6) fprintf(stderr, "[str] dataUsingEncoding enc=0x%x '%s' -> %u bytes\n", enc, (const char *)u, (unsigned)n); }
    return d;
}
static id i_initWithData_encoding(id s, SEL c, id data, unsigned enc) {
    St *d = data ? state(data) : NULL; const unsigned char *b = d ? (const unsigned char *)d->a : NULL; size_t n = d ? (size_t)d->i : 0;
    int unit = enc_unit(enc), be = enc_be(enc); char *o = malloc(n * 4 + 8); size_t m = 0;
    if (enc == 8) { m = sjis_to_utf8(b, n, o); }
    else if (unit == 1) { for (size_t i = 0; i < n; i++) { unsigned cp = b[i]; if (enc == 4) o[m++] = (char)cp; else m += (size_t)u8_put(o + m, cp); } }
    else { size_t i = 0; if (n >= (size_t)unit && enc_bom(enc)) i = (size_t)unit;
        for (; i + (size_t)unit <= n; i += (size_t)unit) {
            unsigned cp = 0; if (unit == 2) cp = be ? (b[i] << 8 | b[i + 1]) : (b[i + 1] << 8 | b[i]);
            else { for (int k = 0; k < 4; k++) cp |= (unsigned)b[i + (size_t)k] << (be ? (3 - k) : k) * 8; }
            if (unit == 2 && cp >= 0xD800 && cp < 0xDC00 && i + 4 <= n) { unsigned lo = be ? (b[i + 2] << 8 | b[i + 3]) : (b[i + 3] << 8 | b[i + 2]); cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00); i += 2; }
            m += (size_t)u8_put(o + m, cp);
        } }
    o[m] = 0; id r = nsstr_new(o); free(o);
    { static int k; if (k++ < 6) fprintf(stderr, "[str] initWithData enc=0x%x %u bytes -> '%s'\n", enc, (unsigned)n, nsstr_utf8(r)); }
    return r;
}
static int g_strconv;
static id i_cStringUsingEncoding(id s, SEL c, unsigned enc) {
    if (enc != 8) return (id)nsstr_utf8(s);
    static unsigned char *ring[64]; static int ri;                 /* callers use the pointer briefly: keep the last 64 */
    const char *u = nsstr_utf8(s); free(ring[ri]); ring[ri] = malloc(strlen(u) * 2 + 2);
    size_t n = sjis_from_utf8(u, ring[ri]); ring[ri][n] = 0; id r = (id)ring[ri]; ri = (ri + 1) & 63;
    { static int k; if (k++ < 6) fprintf(stderr, "[str] cStringUsingEncoding(SJIS) '%s' -> %u bytes\n", u, (unsigned)n); }
    return r;
}
static id c_dataWithBytes_length(id cls, SEL c, const void *p, unsigned n) { return nsdata_new(p, n); }
static id i_data_bytes(id d, SEL c) { return (id)state(d)->a; }
static id i_data_length(id d, SEL c) { return (id)(uintptr_t)state(d)->i; }
static id i_data_getBytes_length(id d, SEL c, void *buf, unsigned n) { St *s = state(d); unsigned k = n < (unsigned)s->i ? n : (unsigned)s->i; if (buf && k) memcpy(buf, s->a, k); return 0; }

/* NSBundle / paths */
static id c_mainBundle(id cls, SEL c) { static id b; if (!b) b = new_obj(U32(cls)); return b; }
static id i_bundlePath(id b, SEL c) { return nsstr_new(GAME_DIR_NS); }
static id i_resourcePath(id b, SEL c) { return nsstr_new(GAME_DIR_NS); }
static id i_pathForResource(id b, SEL c, id name, id type) {
    char buf[512]; snprintf(buf, sizeof buf, GAME_DIR "%s%s%s", nsstr_utf8(name), type ? "." : "", type ? nsstr_utf8(type) : ""); return nsstr_new(buf);
}

/* UIApplication */
static id c_sharedApplication(id cls, SEL c) { static id a; if (!a) a = new_obj(U32(cls)); return a; }
static id i_setDelegate(id a, SEL c, id d) { state(a)->a = d; return 0; }
static id i_delegate(id a, SEL c) { return state(a)->a; }
/* UIViewController / UIView */
static id i_vc_setView(id v, SEL c, id view) { state(v)->a = view; return 0; }
static id i_vc_view(id v, SEL c) { return state(v)->a; }
static id i_initWithNibName(id v, SEL c, id n, id b) { return v; }
static id i_view_layer(id v, SEL c) { St *s = state(v); if (!s->b) s->b = new_obj(U32(native_class("CAEAGLLayer", NULL))); return s->b; }
static id c_zero_cls(id cls, SEL c) { return 0; }
/* EAGL */
static id i_initWithAPI(id o, SEL c, id api) { return o; }
static id c_setCurrentContext(id cls, SEL c, id ctx) { return (id)1; }
/* NSTimer */
typedef struct { id timer, target; SEL sel; double interval, next; int repeats, valid; } Timer;
volatile unsigned g_exec_count;
static Timer timers[64]; static int n_timers;
/* run-loop clock: the process timer (monotonic, microsecond). gettimeofday() on the Vita can step, which left the
 * run loop idling in 1 ms naps for ~50 polls until the clock caught up with the timer's next fire time (update155). */
extern unsigned long long gl_prof_now(void);
static double now(void) { return (double)gl_prof_now() * 1e-6; }
static id SOFT c_scheduledTimer(id cls, SEL c, double iv, id target, SEL sel, id info, int repeats) {
    Timer *t = &timers[n_timers < 64 ? n_timers++ : 63];
    t->timer = new_obj(U32(cls)); t->target = target; t->sel = sel; t->interval = iv; t->next = now() + iv; t->repeats = repeats; t->valid = 1; return t->timer;
}
static id i_invalidate(id o, SEL c) { for (int i = 0; i < n_timers; i++) if (timers[i].timer == o) timers[i].valid = 0; return 0; }
/* Movie playback is not emulated: when the game starts a movie and registers for the "playback finished" notification,
 * deliver it to the observer ~1.5 s later so the game carries on past the intro video. */
static int g_movie_active;
typedef struct { id obs; SEL sel; double fire; int valid; } PendNote;
static PendNote pend_notes[16];
static id i_mp_init(id o, SEL c, id url) { g_movie_active = 1; fprintf(stderr, "[movie] play requested (skipping)\n"); fflush(stderr); return o; }
static id i_addObserver(id nc, SEL c, id obs, SEL sel, id name, id obj) {
    if (g_movie_active) {
        for (int i = 0; i < 16; i++) if (!pend_notes[i].valid) { pend_notes[i].obs = obs; pend_notes[i].sel = sel; pend_notes[i].fire = now() + 1.5; pend_notes[i].valid = 1; break; }
        g_movie_active = 0;
        fprintf(stderr, "[movie] observer registered -> will deliver 'finished' to selector '%s'\n", (uintptr_t)sel > 0x10000 ? (const char *)sel : "?"); fflush(stderr);
    }
    return 0;
}
/* NSCoder (nib decoding is not emulated: every decode returns nil/0) */

__attribute__((weak)) int platform_poll_quit(void) { return 0; }


/* ------------------------------------------------------------------ touch input (Vita front panel -> UITouch/UIEvent for AlchemyView) */
extern int platform_touch_poll(int max, int *ids, float *xs, float *ys);
extern void gl_screen_to_game(float sx, float sy, float *gx, float *gy);
#define MAXT 10
typedef struct { int active, seen, vid, phase; float x, y; id obj; } TSlot;
static TSlot tslot[MAXT];
static id g_view, g_event, g_set_all, g_set_chg;
static unsigned i_set_enum(id set, SEL c, u32 *st, id *buf, unsigned len) {
    St *s = state(set);
    if (st[0] != 0) return 0;
    static unsigned long mut;
    st[0] = 1; st[1] = U32(s->a); st[2] = U32(&mut);
    return (unsigned)s->i;
}
static id i_set_any(id set, SEL c) { St *s = state(set); return s->i ? ((id *)s->a)[0] : 0; }
static id i_touch_phase(id t, SEL c) { TSlot *k = state(t)->a; return (id)(intptr_t)(k ? k->phase : 3); }
static void i_touch_loc(float *ret, id t, SEL c, id view) { TSlot *k = state(t)->a; ret[0] = k ? k->x : 0; ret[1] = k ? k->y : 0; }
static id i_event_allTouches(id e, SEL c) { return g_set_all; }
static id i_event_touchesForView(id e, SEL c, id v) { return g_set_all; }
static id i_touch_one(id t, SEL c) { return (id)1; }
static void fill_set(id set, int phase) {   /* phase -1 = every active touch */
    St *s = state(set); if (!s->a) s->a = calloc(MAXT, sizeof(id));
    int n = 0; for (int i = 0; i < MAXT; i++) if (tslot[i].active && (phase < 0 || tslot[i].phase == phase)) ((id *)s->a)[n++] = tslot[i].obj;
    s->i = n;
}
static void touch_poll_dispatch(void) {
    if (!g_view) return;
    if (!g_event) {
        g_event = new_obj(U32(native_class("UIEvent", NULL)->ptr));
        g_set_all = new_obj(U32(native_class("NSSet", NULL)->ptr)); g_set_chg = new_obj(U32(native_class("NSSet", NULL)->ptr));
        for (int i = 0; i < MAXT; i++) { tslot[i].obj = new_obj(U32(native_class("UITouch", NULL)->ptr)); state(tslot[i].obj)->a = &tslot[i]; }
    }
    int ids[MAXT]; float xs[MAXT], ys[MAXT];
    int n = platform_touch_poll(MAXT, ids, xs, ys);
    int began = 0, moved = 0, ended = 0;
    for (int i = 0; i < MAXT; i++) tslot[i].seen = 0;
    for (int r = 0; r < n; r++) {
        float gx, gy; gl_screen_to_game(xs[r], ys[r], &gx, &gy);
        TSlot *k = 0;
        for (int i = 0; i < MAXT; i++) if (tslot[i].active && tslot[i].vid == ids[r]) { k = &tslot[i]; break; }
        if (!k) {
            for (int i = 0; i < MAXT; i++) if (!tslot[i].active) { k = &tslot[i]; break; }
            if (!k) continue;
            k->active = 1; k->vid = ids[r]; k->x = gx; k->y = gy; k->phase = 0; began = 1;
            fprintf(stderr, "[touch] began id=%d at %d,%d\n", ids[r], (int)gx, (int)gy); fflush(stderr);
        } else if (gx - k->x > 0.5f || k->x - gx > 0.5f || gy - k->y > 0.5f || k->y - gy > 0.5f) { k->x = gx; k->y = gy; k->phase = 1; moved = 1; }
        else k->phase = 2;
        k->seen = 1;
    }
    for (int i = 0; i < MAXT; i++) if (tslot[i].active && !tslot[i].seen) { tslot[i].phase = 3; ended = 1; }
    if (began || moved || ended) {
        fill_set(g_set_all, -1);
        if (began) { fill_set(g_set_chg, 0); msg(g_view, "touchesBegan:withEvent:", g_set_chg, g_event, 0); }
        if (moved) { fill_set(g_set_chg, 1); msg(g_view, "touchesMoved:withEvent:", g_set_chg, g_event, 0); }
        if (ended) { fill_set(g_set_chg, 3); msg(g_view, "touchesEnded:withEvent:", g_set_chg, g_event, 0); fprintf(stderr, "[touch] ended\n"); fflush(stderr); }
    }
    for (int i = 0; i < MAXT; i++) if (tslot[i].active && tslot[i].phase == 3) tslot[i].active = 0;
}

/* ---- main-thread work queue: performSelectorOnMainThread / performSelector:afterDelay: (BB3's game thread posts its
 *      online/IGP updates this way). Drained by the run loop each pass. ---- */
static pthread_t g_main_thr; static int g_main_thr_set;
typedef struct { id target; SEL sel; id arg; double at; volatile int state; } MainJob;   /* state 0 free, 1 queued, 2 running */
static MainJob mq[64]; static volatile int mq_lk;
static int on_main(void) { return g_main_thr_set && pthread_equal(pthread_self(), g_main_thr); }
static double now(void);
static int mq_post(id t, SEL sel, id arg, double delay) {
    while (__atomic_exchange_n(&mq_lk, 1, __ATOMIC_ACQUIRE)) usleep(1);
    int k = -1; for (int i = 0; i < 64; i++) if (!mq[i].state) { k = i; break; }
    if (k >= 0) { mq[k].target = t; mq[k].sel = sel; mq[k].arg = arg; mq[k].at = now() + delay; __atomic_store_n(&mq[k].state, 1, __ATOMIC_RELEASE); }
    __atomic_store_n(&mq_lk, 0, __ATOMIC_RELEASE);
    if (k < 0) { static int n; if (n++ < 5) { fprintf(stderr, "[objc] main queue full, dropping %s\n", sel); fflush(stderr); } }
    return k;
}
static void mq_drain(void) {
    double t = now();
    for (int i = 0; i < 64; i++) if (mq[i].state == 1 && mq[i].at <= t) {
        mq[i].state = 2; msg(mq[i].target, mq[i].sel, mq[i].arg, 0, 0); __atomic_store_n(&mq[i].state, 0, __ATOMIC_RELEASE);
    }
}
static id i_performOnMain(id o, SEL c, SEL sel, id arg, int wait) {
    { static int n; if (n < 20) { n++; fprintf(stderr, "[objc] performSelectorOnMainThread %s wait=%d\n", sel, wait); fflush(stderr); } }
    if (on_main() || !g_main_thr_set) return msg(o, sel, arg, 0, 0);
    int k = mq_post(o, sel, arg, 0);
    if (wait && k >= 0) while (mq[k].state && mq[k].target == o) usleep(500);
    return 0;
}
static id SOFT i_performAfterDelay(id o, SEL c, SEL sel, id arg, double delay) { mq_post(o, sel, arg, delay); return 0; }
static id i_perform0(id o, SEL c, SEL sel) { return msg(o, sel, 0, 0, 0); }
static id i_perform1(id o, SEL c, SEL sel, id a) { return msg(o, sel, a, 0, 0); }
static id i_perform2(id o, SEL c, SEL sel, id a, id b) { return msg(o, sel, a, b, 0); }
static id c_isMainThread(id cls, SEL c) { return (id)(uintptr_t)on_main(); }
static id c_currentThread(id cls, SEL c) { static id t; if (!t) t = new_obj(U32(cls)); return t; }
static id SOFT c_sleepForTimeInterval(id cls, SEL c, double s) { if (s > 0) usleep((useconds_t)(s * 1e6)); return 0; }
typedef struct { id t; SEL s; id a; } ThrArg;
static void *thr_entry(void *p) { ThrArg a = *(ThrArg *)p; free(p); fprintf(stderr, "[objc] NSThread start [%p %s]\n", (void *)a.t, a.s); fflush(stderr); msg(a.t, a.s, a.a, 0, 0); return NULL; }
static id c_detachNewThread(id cls, SEL c, SEL sel, id target, id arg) {
    ThrArg *a = malloc(sizeof *a); a->t = target; a->s = sel; a->a = arg;
    pthread_t th; pthread_attr_t at; pthread_attr_init(&at); pthread_attr_setstacksize(&at, 0x100000);
    pthread_create(&th, &at, thr_entry, a); return 0;
}
static id i_bundleIdentifier(id b, SEL c) { return nsstr_new(GAME_BUNDLE_ID); }

void *objc_nsstr(const char *u) { return nsstr_new(u); }
const char *objc_nsstr_utf8(void *s) { return nsstr_utf8(s); }

static int UIApplicationMain_impl(int argc, char **argv, id principal, id delegateName) {
    fprintf(stderr, "[objc] UIApplicationMain (classes loaded: %d)\n", rc_n); fflush(stderr);
    g_main_thr = pthread_self(); g_main_thr_set = 1;
    /* emulate MainWindow.nib (classes from game.h): app delegate, window, the game's GL view built via initWithCoder:,
     * optionally a view controller owning the view; outlets are wired with the usual setters when the delegate has them */
    RClass *dc = class_named(GAME_DELEGATE), *vwc = class_named(GAME_VIEW), *vcc = GAME_VC ? class_named(GAME_VC) : NULL;
    if (!dc || !vwc) { fprintf(stderr, "[objc] missing game classes (%s %s)\n", GAME_DELEGATE, GAME_VIEW); return 1; }
    id app = msg(PTR(native_class("UIApplication", NULL)->ptr), "sharedApplication", 0, 0, 0);
    id delegate = msg(msg(PTR(dc->ptr), "alloc", 0, 0, 0), "init", 0, 0, 0);
    msg(app, "setDelegate:", delegate, 0, 0);
    id window = msg(msg(PTR(native_class("UIWindow", "UIView")->ptr), "alloc", 0, 0, 0), "init", 0, 0, 0);
    msg(delegate, "setWindow:", window, 0, 0);
    id coder = new_obj(U32(native_class("NSCoder", NULL)->ptr));
    id view = msg(msg(PTR(vwc->ptr), "alloc", 0, 0, 0), "initWithCoder:", coder, 0, 0);
    if (vcc) {
        id vc = msg(msg(PTR(vcc->ptr), "alloc", 0, 0, 0), "initWithNibName:bundle:", 0, 0, 0);
        msg(delegate, "setViewController:", vc, 0, 0);
        msg(vc, "setView:", view, 0, 0);
    } else {
        msg(window, "addSubview:", view, 0, 0);
        msg(delegate, "setGlView:", view, 0, 0);
    }
    g_view = view;
    fprintf(stderr, "[objc] nib: delegate %p window %p view %p\n", (void *)delegate, (void *)window, (void *)view);
    fprintf(stderr, "[objc] calling application:didFinishLaunchingWithOptions:\n"); fflush(stderr);
    msg(delegate, "application:didFinishLaunchingWithOptions:", app, 0, 0);
    /* iOS then brings the app to the foreground: BB3's game thread waits for the flag this sets */
    fprintf(stderr, "[objc] calling applicationDidBecomeActive:\n"); fflush(stderr);
    msg(delegate, "applicationDidBecomeActive:", app, 0, 0);
    { extern int g_main_core; extern int sceKernelChangeThreadCpuAffinityMask(int thid, int mask) __attribute__((weak));
      if (sceKernelChangeThreadCpuAffinityMask && g_main_core >= 0 && g_main_core <= 2) sceKernelChangeThreadCpuAffinityMask(0, 0x10000 << g_main_core); }   /* UI/run-loop thread off the engine core */
    fprintf(stderr, "[objc] launched; entering run loop (%d timers)\n", n_timers); fflush(stderr);
    for (int i = 0; i < n_timers; i++) fprintf(stderr, "[objc] timer %d target=%p sel=%p '%s' iv=%g\n", i, (void *)timers[i].target, (const void *)timers[i].sel, (uintptr_t)timers[i].sel > 0x10000 ? (const char *)timers[i].sel : "?", timers[i].interval);
    g_rt_running = 1;
    for (;;) {
        { extern unsigned long long gl_prof_now(void); extern void gl_note_poll(unsigned long long us);
          unsigned long long p0 = gl_prof_now();
          if (platform_poll_quit()) {   /* like iOS on Home / quit: let the game pause and save before the process ends */
              /* resign-active pauses the game and runs its pause/save path. background/terminate are NOT sent: they tear
               * objects down while the game thread is still running (it crashed on a freed object). Then end the process
               * at once - no static destructors, no game thread touching freed memory. */
              fprintf(stderr, "[objc] quit: sending applicationWillResignActive: so the game pauses and saves\n"); fflush(stderr);
              msg(delegate, "applicationWillResignActive:", app, 0, 0);
              for (int k = 0; k < 50; k++) { mq_drain(); usleep(10000); }   /* ~0.5 s for the game's save work */
              fflush(NULL);
              fprintf(stderr, "[objc] quit: done\n"); fflush(stderr);
              { extern int sceKernelExitProcess(int); sceKernelExitProcess(0); }
              break;
          }
          touch_poll_dispatch();
          mq_drain();
          gl_note_poll(gl_prof_now() - p0); }
        double t = now(); int fired = 0;
        for (int i = 0; i < 16; i++) if (pend_notes[i].valid && pend_notes[i].fire <= t) {
            pend_notes[i].valid = 0; fprintf(stderr, "[movie] delivering finished notification\n"); fflush(stderr);
            msg(pend_notes[i].obs, pend_notes[i].sel, 0, 0, 0);
        }
        for (int i = 0; i < n_timers; i++) if (timers[i].valid && (timers[i].next <= t || !(timers[i].interval > 0.05))) {   /* frame timers (interval <= 50 ms: the game uses 1/30 s) fire every pass: frame pacing is done once, at present. Two 30 Hz schedulers (this timer, re-armed from its fire time, and the present pacing) beat against each other = the tick */
            Timer *tm = &timers[i]; tm->next = t + tm->interval; if (!tm->repeats) tm->valid = 0;
            { extern unsigned long long gl_prof_now(void); extern volatile unsigned g_present_count; extern void gl_note_exec(unsigned long long us, int presented);
              unsigned pc0 = g_present_count; unsigned long long e0 = gl_prof_now();
              msg(tm->target, tm->sel, tm->timer, 0, 0);
              unsigned long long dur = gl_prof_now() - e0; int drew = g_present_count != pc0;
              gl_note_exec(dur, drew);
              (void)drew; }   /* the game had nothing to do yet: nap instead of spinning core 1 */
            fired++; g_exec_count++;
        }
        if (!fired) {
            static int nlog; if (nlog < 15 && n_timers > 0) { nlog++;
                fprintf(stderr, "[runloop] no timer due: timer0 valid=%d interval=%d us next-now=%d us\n", timers[0].valid, (int)(timers[0].interval * 1e6), (int)((timers[0].next - now()) * 1e6)); fflush(stderr); }
            usleep(1000);
        }
    }
    return 0;
}
static int w_UIApplicationMain(int argc, char **argv, id p, id d) { return UIApplicationMain_impl(argc, argv, p, d); }
static id w_NSClassFromString(id name) { RClass *k = class_named(nsstr_utf8(name)); return k ? PTR(k->ptr) : 0; }
static char *nsformat(const char *f, va_list ap);
static void w_NSLog(id fmt, ...) { va_list ap; va_start(ap, fmt); char *r = nsformat(nsstr_utf8(fmt), ap); va_end(ap); fprintf(stderr, "[NSLog] %s\n", r); fflush(stderr); free(r); }
static id w_objc_setProperty(id self, SEL c, int off, id v, int atomic, int copy) { *(id *)((char *)self + off) = v; return 0; }
static void w_objc_enumerationMutation(id o) { fprintf(stderr, "[objc] mutation during enumeration\n"); abort(); }


/* ------------------------------------------------------------------ more Foundation / UIKit stand-ins */
#include <sys/stat.h>
static int is_nsstr(id o) { return o && *(u32 *)o == U32(cfstring_class_ptr); }
static id nsnum_new(int v) { id o = new_obj(U32(native_class("NSNumber", NULL)->ptr)); state(o)->i = v; return o; }
static int is_nsnum(id o) { return o && *(u32 *)o == U32(native_class("NSNumber", NULL)->ptr); }

/* printf-style formatter with %@ support; guest varargs follow plain AAPCS core-register rules */
static char *nsformat(const char *f, va_list ap) {
    size_t cap = 1024, n = 0; char *out = malloc(cap);
#define PUTS(str, len) do { size_t l_ = (len); while (n + l_ + 1 > cap) { cap *= 2; out = realloc(out, cap); } memcpy(out + n, (str), l_); n += l_; out[n] = 0; } while (0)
    out[0] = 0;
    while (*f) {
        if (*f != '%') { PUTS(f, 1); f++; continue; }
        f++;
        char spec[40]; int sl = 0; spec[sl++] = '%'; int ll = 0;
        while (*f && strchr("-+ #0123456789.*hlqLzjt", *f) && sl < 30) {
            if (*f == '*') { sl += snprintf(spec + sl, 8, "%d", va_arg(ap, int)); }
            else if (*f == 'l' || *f == 'q' || *f == 'j') { ll++; if (*f != 'l' || ll == 2) { /* 64-bit */ spec[sl++] = *f; } else spec[sl++] = *f; }
            else spec[sl++] = *f;
            f++;
        }
        char conv = *f ? *f++ : 0; char tmp[512]; spec[sl] = 0;
        if (conv == '%') { PUTS("%", 1); continue; }
        if (conv == '@') {
            id o = va_arg(ap, id);
            const char *t = is_nsstr(o) ? nsstr_utf8(o) : is_nsnum(o) ? (snprintf(tmp, sizeof tmp, "%d", state(o)->i), tmp) : o ? "<object>" : "(null)";
            PUTS(t, strlen(t)); continue;
        }
        spec[sl++] = conv; spec[sl] = 0;
        int len = 0;
        switch (conv) {
        case 'd': case 'i': case 'u': case 'x': case 'X': case 'o': case 'c':
            if (ll >= 2 || strstr(spec, "q") || strstr(spec, "j")) len = snprintf(tmp, sizeof tmp, spec, va_arg(ap, long long));
            else len = snprintf(tmp, sizeof tmp, spec, va_arg(ap, int));
            break;
        case 'f': case 'F': case 'e': case 'E': case 'g': case 'G': len = snprintf(tmp, sizeof tmp, spec, va_arg(ap, double)); break;
        case 's': { const char *sp = va_arg(ap, const char *); len = snprintf(tmp, sizeof tmp, spec, sp ? sp : "(null)"); break; }
        case 'p': len = snprintf(tmp, sizeof tmp, spec, va_arg(ap, void *)); break;
        default: len = snprintf(tmp, sizeof tmp, "%s", spec); break;
        }
        if (len > (int)sizeof tmp - 1) len = sizeof tmp - 1;
        if (len > 0) PUTS(tmp, (size_t)len);
    }
    return out;
}
static id c_stringWithFormat(id cls, SEL c, id fmt, ...) {
    va_list ap; va_start(ap, fmt); char *r = nsformat(nsstr_utf8(fmt), ap); va_end(ap);
    id s = nsstr_new(r); free(r); return s;
}
static id i_stringByAppendingString(id s, SEL c, id o) {
    const char *a = nsstr_utf8(s), *b = nsstr_utf8(o); char *t = malloc(strlen(a) + strlen(b) + 1); strcpy(t, a); strcat(t, b);
    id r = nsstr_new(t); free(t); return r;
}
static id i_stringByAppendingPathComponent(id s, SEL c, id o) {
    const char *a = nsstr_utf8(s), *b = nsstr_utf8(o); char *t = malloc(strlen(a) + strlen(b) + 2); strcpy(t, a);
    if (*a && a[strlen(a) - 1] != '/') strcat(t, "/");
    strcat(t, b); id r = nsstr_new(t); free(t); return r;
}
static id i_stringByDeletingLastPathComponent(id s, SEL c) {
    char *t = strdup(nsstr_utf8(s)); char *sl = strrchr(t, '/'); if (sl) *sl = 0; else t[0] = 0;
    id r = nsstr_new(t); free(t); return r;
}
static id i_isEqualToString(id s, SEL c, id o) { return (id)(uintptr_t)(o && !strcmp(nsstr_utf8(s), nsstr_utf8(o))); }
static id i_intValue(id s, SEL c) { return is_nsnum(s) ? (id)(uintptr_t)state(s)->i : (id)(uintptr_t)atoi(nsstr_utf8(s)); }
static id i_str_self(id s, SEL c, id a) { return s; }
static id i_compare_options(id s, SEL c, id o, id opts) { int r = strcmp(nsstr_utf8(s), nsstr_utf8(o)); return (id)(intptr_t)(r < 0 ? -1 : r > 0 ? 1 : 0); }
static id c_numberWithBool(id cls, SEL c, int v) { return nsnum_new(v); }
static id c_numberWithInt(id cls, SEL c, int v) { return nsnum_new(v); }
static id i_boolValue(id n, SEL c) { return (id)(uintptr_t)state(n)->i; }
static id c_URLWithString(id cls, SEL c, id s) { id o = new_obj(U32(cls)); state(o)->a = s; return o; }
static id i_url_path(id u, SEL c) { return state(u)->a; }
static id i_url_absoluteString(id u, SEL c) { return state(u)->a; }

/* NSArray (tiny) */
static id array_of(const char *const *items, int n) {
    id a = new_obj(U32(native_class("NSArray", NULL)->ptr)); St *s = state(a); s->i = n; s->a = calloc(n ? n : 1, sizeof(id));
    for (int i = 0; i < n; i++) ((id *)s->a)[i] = nsstr_new(items[i]);
    return a;
}
static id i_count(id a, SEL c) { return (id)(uintptr_t)state(a)->i; }
static id i_objectAtIndex(id a, SEL c, unsigned i) { St *s = state(a); return i < (unsigned)s->i ? ((id *)s->a)[i] : 0; }
/* NSSearchPathForDirectoriesInDomains(dir, domainMask, expandTilde): every directory maps to the app data folder */
static id w_NSSearchPathForDirectoriesInDomains(unsigned dir, unsigned mask, int tilde) {
    (void)dir; (void)mask; (void)tilde; static const char *const l[] = {GAME_DIR_NS}; return array_of(l, 1);
}
static id c_preferredLanguages(id cls, SEL c) { static const char *const l[] = {"en"}; return array_of(l, 1); }

/* NSUserDefaults (in-memory) */
typedef struct { char *key; id val; } Pref;
static Pref prefs[128]; static int n_prefs;
static Pref *pref_find(const char *k, int create) {
    for (int i = 0; i < n_prefs; i++) if (!strcmp(prefs[i].key, k)) return &prefs[i];
    if (!create || n_prefs >= 128) return NULL;
    prefs[n_prefs].key = strdup(k); return &prefs[n_prefs++];
}
static id c_standardUserDefaults(id cls, SEL c) { static id d; if (!d) d = new_obj(U32(cls)); return d; }
static id i_setObject_forKey(id d, SEL c, id v, id k) { Pref *p = pref_find(nsstr_utf8(k), 1); if (p) p->val = v; return 0; }
static id i_setInteger_forKey(id d, SEL c, int v, id k) { Pref *p = pref_find(nsstr_utf8(k), 1); if (p) p->val = nsnum_new(v); return 0; }
static id i_stringForKey(id d, SEL c, id k) { Pref *p = pref_find(nsstr_utf8(k), 0); return p && is_nsstr(p->val) ? p->val : 0; }
static id i_integerForKey(id d, SEL c, id k) { Pref *p = pref_find(nsstr_utf8(k), 0); return p && is_nsnum(p->val) ? (id)(uintptr_t)state(p->val)->i : 0; }
static id i_synchronize(id d, SEL c) { return (id)1; }

/* NSFileManager */
static id c_defaultManager(id cls, SEL c) { static id m; if (!m) m = new_obj(U32(cls)); return m; }
static id i_attributesOfItemAtPath(id m, SEL c, id path, id err) {
    struct stat st; id d = new_obj(U32(native_class("NSDictionary", NULL)->ptr));
    if (stat(nsstr_utf8(path), &st) == 0) state(d)->d = (double)st.st_size; else { state(d)->d = 0; }
    return d;
}
static unsigned long long i_fileSize(id d, SEL c) { return (unsigned long long)state(d)->d; }

/* AVAudioPlayer -> audio.c mixer (one voice per player; the game never uses the delegate callbacks) */
extern void *snd_player_new(const char *url); extern int snd_play(void *h); extern void snd_stop(void *h); extern int snd_is_playing(void *h);
extern void snd_set_volume(void *h, float v); extern float snd_get_volume(void *h); extern void snd_set_loops(void *h, int n); extern void snd_set_pan(void *h, float p);
static const char *url_path(id u) { if (!u) return ""; St *s = state(u); return s->a ? nsstr_utf8(s->a) : nsstr_utf8(u); }
/* The game calls these on its own stack, which iOS only keeps 4-byte aligned; Vita library code (newlib fopen, pthread_create, ...)
   faults on that (first sound load crashed in _fopen_r). File/thread work is therefore run on an 8-byte aligned stack. */
#if defined(__arm__)
__attribute__((naked, noinline)) static void *aligned_call(void *a, void *b, void *fn) {
    __asm__ volatile(".syntax unified\n"
                     "push {r4, lr}\n"
                     "mov r4, sp\n"
                     "bic r3, r4, #7\n"
                     "mov sp, r3\n"
                     "blx r2\n"
                     "mov sp, r4\n"
                     "pop {r4, pc}\n");
}
#else
static void *aligned_call(void *a, void *b, void *fn) { return ((void *(*)(void *, void *))fn)(a, b); }
#endif
extern unsigned long long gl_prof_now(void);
static void *w_player_new(void *path, void *u) { (void)u; unsigned long long t = gl_prof_now(); void *r = snd_player_new((const char *)path); t = gl_prof_now() - t;
    { static int n; if (t > 2000 && n < 40) { n++; fprintf(stderr, "[snd] SLOW new player %.1f ms %s\n", t / 1000.0, (const char *)path); fflush(stderr); } } return r; }
static void *w_play(void *h, void *u) { (void)u; unsigned long long t = gl_prof_now(); int r = snd_play(h); t = gl_prof_now() - t;
    { static int n; if (t > 2000 && n < 40) { n++; fprintf(stderr, "[snd] SLOW play %.1f ms\n", t / 1000.0); fflush(stderr); } } return (void *)(intptr_t)r; }
static id i_avp_init(id o, SEL c, id url, id *err) { if (err) *err = 0; void *v = aligned_call((void *)url_path(url), 0, (void *)w_player_new); if (!v) return 0; state(o)->c = v; return o; }
static id i_avp_play(id o, SEL c) { return (id)aligned_call(state(o)->c, 0, (void *)w_play); }
static id i_avp_stop(id o, SEL c) { snd_stop(state(o)->c); return 0; }
static id i_avp_isPlaying(id o, SEL c) { return (id)(uintptr_t)snd_is_playing(state(o)->c); }
static void SOFT i_avp_setVolume(id o, SEL c, float v) { snd_set_volume(state(o)->c, v); }
static float SOFT i_avp_volume(id o, SEL c) { return snd_get_volume(state(o)->c); }
static id i_avp_setLoops(id o, SEL c, int n) { snd_set_loops(state(o)->c, n); return 0; }
static void SOFT i_avp_setPan(id o, SEL c, float p) { snd_set_pan(state(o)->c, p); }
static id i_avp_setDelegate(id o, SEL c, id d) { state(o)->b = d; return 0; }

/* UIDevice / UIScreen / misc */
static id c_currentDevice(id cls, SEL c) { static id d; if (!d) d = new_obj(U32(cls)); return d; }
static id i_dev_model(id d, SEL c) { return nsstr_new("iPhone"); }
static id i_dev_systemVersion(id d, SEL c) { return nsstr_new("6.1.3"); }
static id i_dev_uniqueIdentifier(id d, SEL c) { return nsstr_new("0000000000000000000000000000000000000001"); }
static id i_dev_orientation(id d, SEL c) { return (id)1; }                      /* UIDeviceOrientationPortrait */
static id c_mainScreen(id cls, SEL c) { static id d; if (!d) d = new_obj(U32(cls)); return d; }
extern float gl_screen_points_w(void), gl_screen_points_h(void), gl_screen_scale(void);
static void i_screen_bounds(float *ret, id s, SEL c) { ret[0] = 0; ret[1] = 0; ret[2] = gl_screen_points_w(); ret[3] = gl_screen_points_h(); }
static float SOFT i_screen_scale(id s, SEL c) { return gl_screen_scale(); }
static id c_date(id cls, SEL c) { return new_obj(U32(cls)); }
static id i_dateFormatter_stringFromDate(id f, SEL c, id d) { return nsstr_new("2013-01-01 00:00:00"); }
static id c_systemTimeZone(id cls, SEL c) { return new_obj(U32(cls)); }
static id c_defaultCenter(id cls, SEL c) { static id d; if (!d) d = new_obj(U32(cls)); return d; }
extern void gl_mark_drawable(void);
extern void gl_present(void);
static id i_layer_renderbufferStorage(id ctx, SEL c, unsigned target, id layer) { gl_mark_drawable(); return (id)1; }
static id i_present(id ctx, SEL c, unsigned target) { gl_present(); return (id)1; }
static id i_url_init(id o, SEL c, id s) { state(o)->a = s; return o; }

static id i_getCString_max_enc(id s, SEL c, char *buf, unsigned max, unsigned enc) {
    (void)enc; const char *u = nsstr_utf8(s); if (!buf || !max) return 0;
    size_t l = strlen(u); if (l >= max) { memcpy(buf, u, max - 1); buf[max - 1] = 0; return 0; } memcpy(buf, u, l + 1); return (id)1; }
static id c_stringWithCString_enc(id cls, SEL c, const char *cs, unsigned enc) { (void)enc; return cs ? nsstr_new(cs) : 0; }
static id i_initWithUTF8String(id o, SEL c, const char *cs) { return cs ? nsstr_new(cs) : 0; }
static id i_getCString(id s, SEL c, char *buf) { if (buf) strcpy(buf, nsstr_utf8(s)); return 0; }

static void setup_natives(void) {
    static int done; if (done) return; done = 1;
    native_class("NSObject", NULL);
    nm_cls("NSObject", "alloc", c_alloc); nm_cls("NSObject", "allocWithZone:", c_allocWithZone); nm_cls("NSObject", "new", c_new);
    nm_cls("NSObject", "class", c_class); nm_cls("NSObject", "instancesRespondToSelector:", i_respondsToSelector);
    nm_inst("NSObject", "init", i_self); nm_inst("NSObject", "retain", i_self); nm_inst("NSObject", "autorelease", i_self);
    nm_inst("NSObject", "copy", i_self); nm_inst("NSObject", "self", i_self); nm_inst("NSObject", "class", i_class);
    nm_inst("NSObject", "isKindOfClass:", i_isKindOfClass); nm_inst("NSObject", "isMemberOfClass:", i_isKindOfClass);
    nm_inst("NSObject", "respondsToSelector:", i_respondsToSelector); nm_inst("NSObject", "isEqual:", i_isEqual); nm_inst("NSObject", "hash", i_hash);
    nm_inst("NSObject", "dealloc", i_zero); nm_inst("NSObject", "release", i_zero);
    native_class("NSString", "NSObject"); native_class("NSConstantString", "NSString");
    cfstring_class_ptr = PTR(class_named("NSConstantString")->ptr);
    nm_cls("NSString", "stringWithUTF8String:", c_stringWithUTF8String); nm_cls("NSString", "stringWithString:", c_stringWithString);
    { FILE *sf = fopen(GAME_DIR "strconv.off", "rb");   /* string encoding support (Shift-JIS etc.) is on unless this file exists */
      if (sf) { fclose(sf); fprintf(stderr, "[str] encoding support OFF (strconv.off)\n"); }
      else {
        native_class("NSData", "NSObject");
        nm_inst("NSString", "dataUsingEncoding:allowLossyConversion:", i_dataUsingEncoding); nm_inst("NSString", "initWithData:encoding:", i_initWithData_encoding);
        nm_cls("NSData", "dataWithBytes:length:", c_dataWithBytes_length); nm_inst("NSData", "bytes", i_data_bytes); nm_inst("NSData", "length", i_data_length);
        nm_inst("NSData", "getBytes:length:", i_data_getBytes_length);
        nm_inst("NSString", "cStringUsingEncoding:", i_cStringUsingEncoding); g_strconv = 1;
        fprintf(stderr, "[str] encoding support ON (Shift-JIS)\n"); } }
    nm_inst("NSString", "UTF8String", i_UTF8String); nm_inst("NSString", "length", i_length); if (!g_strconv) nm_inst("NSString", "cStringUsingEncoding:", i_UTF8String);
    nm_cls("NSBundle", "mainBundle", c_mainBundle); nm_inst("NSBundle", "bundlePath", i_bundlePath); nm_inst("NSBundle", "resourcePath", i_resourcePath);
    nm_inst("NSBundle", "pathForResource:ofType:", i_pathForResource);
    nm_cls("UIApplication", "sharedApplication", c_sharedApplication); nm_inst("UIApplication", "setDelegate:", i_setDelegate); nm_inst("UIApplication", "delegate", i_delegate);
    native_class("UIView", "NSObject"); native_class("UIViewController", "NSObject"); native_class("UIWindow", "UIView");
    nm_inst("UIView", "initWithCoder:", i_self); nm_inst("UIView", "initWithFrame:", i_self); nm_inst("UIView", "layer", i_view_layer);
    nm_cls("UIView", "layerClass", c_zero_cls);
    nm_inst("UIViewController", "setView:", i_vc_setView); nm_inst("UIViewController", "view", i_vc_view);
    nm_inst("UIViewController", "initWithNibName:bundle:", i_initWithNibName); nm_inst("UIViewController", "initWithCoder:", i_self);
    nm_inst("EAGLContext", "initWithAPI:", i_initWithAPI); nm_cls("EAGLContext", "setCurrentContext:", c_setCurrentContext);
    nm_cls("NSTimer", "scheduledTimerWithTimeInterval:target:selector:userInfo:repeats:", c_scheduledTimer); nm_inst("NSTimer", "invalidate", i_invalidate);
    native_class("NSAutoreleasePool", "NSObject"); native_class("NSCoder", "NSObject");

    /* --- Foundation basics --- */
    nm_cls("NSString", "stringWithFormat:", c_stringWithFormat);
    nm_inst("NSString", "stringByAppendingString:", i_stringByAppendingString);
    nm_inst("NSString", "stringByAppendingPathComponent:", i_stringByAppendingPathComponent);
    nm_inst("NSString", "stringByDeletingLastPathComponent", i_stringByDeletingLastPathComponent);
    nm_inst("NSString", "isEqualToString:", i_isEqualToString); nm_inst("NSString", "intValue", i_intValue);
    nm_inst("NSString", "stringByAddingPercentEscapesUsingEncoding:", i_str_self);
    nm_inst("NSString", "compare:options:", i_compare_options); nm_inst("NSString", "initWithString:", i_str_self);
    nm_cls("NSNumber", "numberWithBool:", c_numberWithBool); nm_cls("NSNumber", "numberWithInt:", c_numberWithInt);
    nm_inst("NSNumber", "intValue", i_boolValue); nm_inst("NSNumber", "boolValue", i_boolValue);
    nm_cls("NSURL", "URLWithString:", c_URLWithString); nm_cls("NSURL", "fileURLWithPath:", c_URLWithString);
    nm_inst("NSURL", "path", i_url_path); nm_inst("NSURL", "absoluteString", i_url_absoluteString); nm_inst("NSURL", "initWithString:", i_url_init);
    nm_inst("NSArray", "count", i_count); nm_inst("NSArray", "objectAtIndex:", i_objectAtIndex);
    nm_cls("NSLocale", "preferredLanguages", c_preferredLanguages);
    nm_cls("NSUserDefaults", "standardUserDefaults", c_standardUserDefaults);
    nm_inst("NSUserDefaults", "setObject:forKey:", i_setObject_forKey); nm_inst("NSUserDefaults", "setInteger:forKey:", i_setInteger_forKey);
    nm_inst("NSUserDefaults", "stringForKey:", i_stringForKey); nm_inst("NSUserDefaults", "integerForKey:", i_integerForKey);
    nm_inst("NSUserDefaults", "synchronize", i_synchronize);
    nm_cls("NSFileManager", "defaultManager", c_defaultManager); nm_inst("NSFileManager", "attributesOfItemAtPath:error:", i_attributesOfItemAtPath);
    nm_inst("NSDictionary", "fileSize", i_fileSize);
    nm_inst("AVAudioPlayer", "initWithContentsOfURL:error:", i_avp_init); nm_inst("AVAudioPlayer", "play", i_avp_play);
    nm_inst("AVAudioPlayer", "stop", i_avp_stop); nm_inst("AVAudioPlayer", "pause", i_avp_stop); nm_inst("AVAudioPlayer", "isPlaying", i_avp_isPlaying);
    nm_inst("AVAudioPlayer", "setVolume:", i_avp_setVolume); nm_inst("AVAudioPlayer", "volume", i_avp_volume);
    nm_inst("AVAudioPlayer", "setNumberOfLoops:", i_avp_setLoops); nm_inst("AVAudioPlayer", "setPan:", i_avp_setPan);
    nm_inst("AVAudioPlayer", "setDelegate:", i_avp_setDelegate); nm_inst("AVAudioPlayer", "prepareToPlay", i_one);
    nm_cls("UIDevice", "currentDevice", c_currentDevice); nm_inst("UIDevice", "model", i_dev_model);
    nm_inst("UIDevice", "systemVersion", i_dev_systemVersion); nm_inst("UIDevice", "uniqueIdentifier", i_dev_uniqueIdentifier);
    nm_inst("UIDevice", "orientation", i_dev_orientation);
    nm_cls("UIScreen", "mainScreen", c_mainScreen); nm_inst("UIScreen", "bounds", i_screen_bounds); nm_inst("UIScreen", "scale", i_screen_scale);
    nm_cls("NSDate", "date", c_date); nm_inst("NSDateFormatter", "stringFromDate:", i_dateFormatter_stringFromDate);
    nm_cls("NSTimeZone", "systemTimeZone", c_systemTimeZone); nm_cls("NSNotificationCenter", "defaultCenter", c_defaultCenter);
    native_class("MPMoviePlayerViewController", "NSObject");
    nm_inst("MPMoviePlayerViewController", "initWithContentURL:", i_mp_init);
    nm_inst("NSNotificationCenter", "addObserver:selector:name:object:", i_addObserver);
    nm_inst("EAGLContext", "renderbufferStorage:fromDrawable:", i_layer_renderbufferStorage);
    nm_inst("EAGLContext", "presentRenderbuffer:", i_present);
    nm_inst("NSSet", "countByEnumeratingWithState:objects:count:", i_set_enum); nm_inst("NSSet", "count", i_count); nm_inst("NSSet", "anyObject", i_set_any);
    nm_inst("UITouch", "phase", i_touch_phase); nm_inst("UITouch", "locationInView:", i_touch_loc); nm_inst("UITouch", "tapCount", i_touch_one);
    nm_inst("UIEvent", "allTouches", i_event_allTouches); nm_inst("UIEvent", "touchesForView:", i_event_touchesForView);
    nm_inst("NSObject", "performSelectorOnMainThread:withObject:waitUntilDone:", i_performOnMain);
    nm_inst("NSObject", "performSelector:withObject:afterDelay:", i_performAfterDelay);
    nm_inst("NSObject", "performSelector:", i_perform0); nm_inst("NSObject", "performSelector:withObject:", i_perform1);
    nm_inst("NSObject", "performSelector:withObject:withObject:", i_perform2);
    nm_cls("NSThread", "isMainThread", c_isMainThread); nm_cls("NSThread", "currentThread", c_currentThread);
    nm_inst("NSThread", "isMainThread", c_isMainThread); nm_cls("NSThread", "sleepForTimeInterval:", c_sleepForTimeInterval);
    nm_cls("NSThread", "detachNewThreadSelector:toTarget:withObject:", c_detachNewThread);
    nm_inst("NSString", "getCString:maxLength:encoding:", i_getCString_max_enc); nm_inst("NSString", "getCString:", i_getCString);
    nm_cls("NSString", "stringWithCString:encoding:", c_stringWithCString_enc); nm_cls("NSString", "stringWithCString:", c_stringWithUTF8String);
    nm_inst("NSString", "initWithUTF8String:", i_initWithUTF8String); nm_inst("NSString", "initWithCString:encoding:", c_stringWithCString_enc);
    nm_inst("NSBundle", "bundleIdentifier", i_bundleIdentifier);
}

void *objc_native_class_symbol(const char *sym) {
    setup_natives();
    if (!strcmp(sym, "___CFConstantStringClassReference")) return PTR(class_named("NSConstantString")->ptr);
    const char *n; int meta = 0;
    if (!strncmp(sym, "_OBJC_CLASS_$_", 14)) n = sym + 14;
    else if (!strncmp(sym, "_OBJC_METACLASS_$_", 18)) { n = sym + 18; meta = 1; }
    else return NULL;
    RClass *k = native_class(n, strcmp(n, "NSObject") ? "NSObject" : NULL);
    return PTR(meta ? k->meta->ptr : k->ptr);
}
void *objc_import_lookup(const char *name) {
    setup_natives();
    if (!strcmp(name, "_objc_msgSend")) return (void *)rt_msgSend;
    if (!strcmp(name, "_objc_msgSend_stret")) return (void *)rt_msgSend_stret;
    if (!strcmp(name, "_objc_msgSendSuper2")) return (void *)rt_msgSendSuper2;
    if (!strcmp(name, "_objc_setProperty")) return (void *)w_objc_setProperty;
    if (!strcmp(name, "_objc_enumerationMutation")) return (void *)w_objc_enumerationMutation;
    if (!strcmp(name, "_UIApplicationMain")) return (void *)w_UIApplicationMain;
    if (!strcmp(name, "_NSClassFromString")) return (void *)w_NSClassFromString;
    if (!strcmp(name, "_NSLog")) return (void *)w_NSLog;
    if (!strcmp(name, "_NSSearchPathForDirectoriesInDomains")) return (void *)w_NSSearchPathForDirectoriesInDomains;
    return NULL;
}
