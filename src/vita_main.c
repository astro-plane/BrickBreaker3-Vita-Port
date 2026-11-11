/* PS Vita entry point for the iOS Mach-O loader (game identity in game.h).
 * Allocation scheme from Rinnegatamante/fink_vita (loader/so_util.c): fixed-address RX block for code, fixed-address RW
 * block for data, writes into RX via kuKernelCpuUnrestrictedMemcpy. */
#include <psp2/kernel/modulemgr.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <kubridge.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "macho.h"
#include "objc_rt.h"
#include <psp2/ctrl.h>
#include <psp2/touch.h>
#include <psp2/power.h>

#ifndef SCE_KERNEL_MEMBLOCK_TYPE_USER_RX
#define SCE_KERNEL_MEMBLOCK_TYPE_USER_RX 0x0C20D050
#endif

#define LOAD_BASE 0x98000000u   /* guest vmaddr vm_low (0x1000) is mapped here */
#define BUILD_TAG "bb3-update031"
/* The game allocates through malloc/new (the C library heap), not sbrk: give that heap the room. The default was too
 * small - new(67 KB) for the music player returned NULL after ~18 MB were in use. */
int _newlib_heap_size_user = 128 * 1024 * 1024;

extern void *shim_for_import(int idx);
extern uint32_t shim_resolve(int idx);

/* staging buffer: all patching happens here in plain RW memory, then it is committed */
uint8_t *platform_alloc_image(uint32_t size) { return calloc(1, size); }
void platform_flush_icache(void *p, uint32_t s) { kuKernelFlushCaches(p, s); }
uint32_t platform_resolve(int idx) { return shim_resolve(idx); }
int platform_poll_quit(void) {
    SceCtrlData pad; sceCtrlPeekBufferPositive(0, &pad, 1);
    return (pad.buttons & SCE_CTRL_START) && (pad.buttons & SCE_CTRL_SELECT);
}

/* ---- state the shared GL/runtime code reads. These drove Baroque's camera work; with no camera hook they stay at
 *      "inactive", which turns every camera tweak in gl_shims off. ---- */
int g_first_person, g_cam_ok, g_view_diag, g_vreal_ok, g_camreal, g_clip_on;
float g_pull_scale, g_cam_cosp = 1.f, g_cam_sinp;
volatile unsigned g_follow_tick;
void vreal_apply(void) {}
static uint32_t g_vm_low = 0x1000;
uint32_t gvm_addr(uint32_t vm) { return LOAD_BASE + vm - g_vm_low; }

/* ---------------- controls ----------------
 * Front touch goes straight to the game (it is a touch game). On top of that the pad drives a virtual finger:
 *   left stick / D-pad left-right : a finger held on the paddle row, moved sideways (vita.cfg paddley=, padspeed=)
 *   cross                          : a tap at the paddle finger's position (launch the ball / press what is under it)
 *   SELECT                         : pointer mode toggle - left stick moves a free pointer, cross taps there
 * Positions are game points (320x480 portrait). vita.cfg pad=0 turns the virtual finger off. */
static int g_pad_on = 1, g_pad_log;
static float g_paddle_y = 420.f, g_pad_speed = 7.f, g_pad_dead = 0.20f, g_fx = 160.f;
static int g_ptr_on; static float g_ptr_x = 160.f, g_ptr_y = 240.f;
int pad_pointer(float *x, float *y) { *x = g_ptr_x; *y = g_ptr_y; return g_ptr_on; }
void pad_cfg_line(const char *line) {
    float v;
    if (sscanf(line, "pad=%f", &v) == 1) g_pad_on = (int)v;
    else if (sscanf(line, "padlog=%f", &v) == 1) g_pad_log = (int)v;
    else if (sscanf(line, "paddley=%f", &v) == 1) g_paddle_y = v;
    else if (sscanf(line, "padspeed=%f", &v) == 1) g_pad_speed = v;
    else if (sscanf(line, "paddead=%f", &v) == 1) g_pad_dead = v;
}
void pad_init(void) {
    sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG_WIDE);
    sceTouchSetSamplingState(SCE_TOUCH_PORT_BACK, SCE_TOUCH_SAMPLING_STATE_STOP);
    fprintf(stderr, "[pad] virtual finger %s, paddle row y=%.0f\n", g_pad_on ? "ON" : "off", g_paddle_y); fflush(stderr);
}
extern void gl_game_to_screen(float gx, float gy, float *nx, float *ny);
extern float gl_screen_points_w(void), gl_screen_points_h(void);
static int pad_virtual(int max, int n, int *ids, float *xs, float *ys) {
    static int fing_on, idle, prev_sel, prev_cross;
    if (!g_pad_on) return n;
    SceCtrlData pad; memset(&pad, 0, sizeof pad); sceCtrlPeekBufferPositive(0, &pad, 1);
    if ((pad.buttons & SCE_CTRL_START) && (pad.buttons & SCE_CTRL_SELECT)) return n;
#define ADD(id_, gx_, gy_) do { if (n < max) { float nx_, ny_; gl_game_to_screen((gx_), (gy_), &nx_, &ny_); ids[n] = (id_); xs[n] = nx_; ys[n] = ny_; n++; } } while (0)
    float W = gl_screen_points_w(), H = gl_screen_points_h();
    float lx = (pad.lx - 128) / 127.f, ly = (pad.ly - 128) / 127.f;
    if (fabsf(lx) < g_pad_dead) lx = 0; if (fabsf(ly) < g_pad_dead) ly = 0;
    if (pad.buttons & SCE_CTRL_LEFT) lx = -1.f; if (pad.buttons & SCE_CTRL_RIGHT) lx = 1.f;
    if (pad.buttons & SCE_CTRL_UP) ly = -1.f; if (pad.buttons & SCE_CTRL_DOWN) ly = 1.f;
    { extern int g_rotate;   /* Vita held sideways: the player's left/right is the Vita's up/down */
      float ux = lx, uy = ly;
      if (g_rotate == 1) { ux = ly; uy = -lx; } else if (g_rotate == 2) { ux = -ly; uy = lx; }
      lx = ux; ly = uy; }
    int sel = (pad.buttons & SCE_CTRL_SELECT) != 0, cross = (pad.buttons & SCE_CTRL_CROSS) != 0;
    if (sel && !prev_sel) { g_ptr_on = !g_ptr_on; fing_on = 0; fprintf(stderr, "[pad] pointer mode %s\n", g_ptr_on ? "ON" : "off"); fflush(stderr); }
    prev_sel = sel;
    if (g_ptr_on) {
        g_ptr_x += lx * g_pad_speed; g_ptr_y += ly * g_pad_speed;
        if (g_ptr_x < 0) g_ptr_x = 0; if (g_ptr_x > W - 1) g_ptr_x = W - 1; if (g_ptr_y < 0) g_ptr_y = 0; if (g_ptr_y > H - 1) g_ptr_y = H - 1;
        if (cross) ADD(240, g_ptr_x, g_ptr_y);
    } else {
        /* paddle finger: down while the stick/D-pad is used, lifted ~0.5 s after it is released */
        if (lx != 0.f) { idle = 0; if (!fing_on) fing_on = 1; g_fx += lx * g_pad_speed * (fabsf(lx) > 0.9f ? 1.5f : 1.f); }
        else if (fing_on && ++idle > 30) fing_on = 0;
        if (g_fx < 0) g_fx = 0; if (g_fx > W - 1) g_fx = W - 1;
        if (fing_on) ADD(200, g_fx, g_paddle_y);
        else if (cross) ADD(201, g_fx, g_paddle_y);
    }
    if (g_pad_log && (cross != prev_cross || lx != 0.f)) { fprintf(stderr, "[pad] buttons=%08x L %d,%d finger %.0f\n", pad.buttons, pad.lx, pad.ly, g_fx); }
    prev_cross = cross;
#undef ADD
    return n;
}
int platform_touch_poll(int max, int *ids, float *xs, float *ys) {
    static int inited;
    if (!inited) { inited = 1; sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, SCE_TOUCH_SAMPLING_STATE_START); }
    SceTouchData td; memset(&td, 0, sizeof td);
    int n = 0;
    if (sceTouchPeek(SCE_TOUCH_PORT_FRONT, &td, 1) > 0) {
        n = td.reportNum < max ? td.reportNum : max;
        for (int i = 0; i < n; i++) { ids[i] = td.report[i].id; xs[i] = td.report[i].x / 1920.0f; ys[i] = td.report[i].y / 1088.0f; }
    }
    return pad_virtual(max, n, ids, xs, ys);
}

/* ---------------- memory ---------------- */
static int alloc_fixed(const char *name, int type, uint32_t addr, uint32_t size) {
    SceKernelAllocMemBlockKernelOpt opt;
    memset(&opt, 0, sizeof opt);
    opt.size = sizeof opt;
    opt.attr = 0x1;
    opt.field_C = addr;
    return kuKernelAllocMemBlock(name, type, (size + 0xfff) & ~0xfffu, &opt);
}
/* heap for the guest allocator's sbrk: same kubridge call as the image (no extra imports) */
void *platform_alloc_heap(uint32_t size) {
    if (alloc_fixed(GAME_TAG "_heap", SCE_KERNEL_MEMBLOCK_TYPE_USER_RW, 0x9a000000u, size) < 0) return NULL;
    return (void *)0x9a000000u;
}
/* segment index 1 = __TEXT (RX), 2 = __DATA (RW) */
static int commit_image(const macho_t *m) {
    uint32_t t_off = m->seg[1].vmaddr - m->vm_low, d_off = m->seg[2].vmaddr - m->vm_low;
    if (alloc_fixed(GAME_TAG "_text", SCE_KERNEL_MEMBLOCK_TYPE_USER_RX, LOAD_BASE + t_off, m->seg[1].vmsize) < 0) return -1;
    if (alloc_fixed(GAME_TAG "_data", SCE_KERNEL_MEMBLOCK_TYPE_USER_RW, LOAD_BASE + d_off, m->seg[2].vmsize) < 0) return -2;
    kuKernelCpuUnrestrictedMemcpy((void *)(LOAD_BASE + t_off), m->base + t_off, m->seg[1].vmsize);
    kuKernelCpuUnrestrictedMemcpy((void *)(LOAD_BASE + d_off), m->base + d_off, m->seg[2].vmsize);
    kuKernelFlushCaches((void *)(LOAD_BASE + t_off), m->seg[1].vmsize);
    return 0;
}

extern void sys_start_watchdog(void);
extern void sys_preallocate_heap(void);
extern void gl_init(void);

/* Calls into guest code save r9 (scratch on iOS armv7, callee-saved for us). */
__attribute__((naked, noinline)) int guest_call2(void *fn, uintptr_t a, uintptr_t b) {
    __asm__ volatile(".syntax unified\n"
        "push {r4, r5, r9, lr}\n"
        "mov r4, r0\n"
        "mov r0, r1\n"
        "mov r1, r2\n"
        "blx r4\n"
        "pop {r4, r5, r9, pc}\n");
}
#undef fflush
static int g_synclog;
int bq_fflush(FILE *f) { if (f == stderr && !g_synclog) return 0; return fflush(f); }
static void *log_flusher(void *arg) {   /* writes the buffered log from core 0 at low priority */
    (void)arg;
    extern int sceKernelChangeThreadPriority(int thid, int prio) __attribute__((weak));
    extern int sceKernelChangeThreadCpuAffinityMask(int thid, int mask) __attribute__((weak));
    if (sceKernelChangeThreadPriority) sceKernelChangeThreadPriority(0, 190);
    if (sceKernelChangeThreadCpuAffinityMask) sceKernelChangeThreadCpuAffinityMask(0, 0x10000);
    for (;;) { usleep(250000); fflush(stderr); }
    return NULL;
}
int main(void) {
    sceIoMkdir(GAME_DIR_NS, 0777);
    FILE *log = freopen(GAME_DIR "loader.txt", "w", stderr); (void)log;
    scePowerSetArmClockFrequency(444); scePowerSetBusClockFrequency(222);
    scePowerSetGpuClockFrequency(222); scePowerSetGpuXbarClockFrequency(166);
    g_synclog = 0;   /* buffered log, written by a background thread (vita.cfg synclog=1: every line immediately - only for crash hunting, it is slow) */
    { FILE *cf = fopen(GAME_DIR "vita.cfg", "r");
      if (cf) { char line[128]; int v; while (fgets(line, sizeof line, cf)) if (sscanf(line, "synclog=%d", &v) == 1) g_synclog = v; fclose(cf); } }
    if (g_synclog) setvbuf(stderr, NULL, _IONBF, 0);
    else { static char logbuf[256 * 1024]; setvbuf(stderr, logbuf, _IOFBF, sizeof logbuf);
           pthread_t th; pthread_attr_t at; pthread_attr_init(&at); pthread_attr_setstacksize(&at, 0x4000); pthread_create(&th, &at, log_flusher, NULL); }
    stdout = stderr;   /* guest puts/printf also land in the log */
    fprintf(stderr, "[" GAME_TAG "] BUILD " BUILD_TAG " %s %s\n", __DATE__, __TIME__);
    const char *path = GAME_BIN;
    SceUID fd = sceIoOpen(path, SCE_O_RDONLY, 0);
    if (fd < 0) { fprintf(stderr, "cannot open %s (%08x) - copy the BBD3 file from Payload/BBD3.app there\n", path, fd); return 1; }
    SceOff len = sceIoLseek(fd, 0, SCE_SEEK_END); sceIoLseek(fd, 0, SCE_SEEK_SET);
    uint8_t *f = malloc(len); sceIoRead(fd, f, len); sceIoClose(fd);

    macho_t m;
    int r = macho_parse(&m, f, len);
    if (r) { fprintf(stderr, "parse failed %d\n", r); return 2; }
    if (m.cryptid) { fprintf(stderr, "the binary is still encrypted (cryptid %d) - it needs a decrypted copy\n", m.cryptid); return 2; }
    g_vm_low = m.vm_low;
    m.final_base = LOAD_BASE;
    if (macho_map(&m, f) || macho_rebase(&m, f) || macho_bind(&m)) { fprintf(stderr, "map/rebase/bind failed\n"); return 3; }
    objc_rt_init(&m);   /* classes + selector fixups must be applied before the image is committed */
    r = commit_image(&m);
    fprintf(stderr, "image committed (%d), rebased %u, bound %u\n", r, m.n_rebased, m.n_bound); fflush(stderr);
    if (r) return 4;

    /* sys_preallocate_heap(): only for games whose own allocator grows with sbrk (Baroque); BB3 does not import sbrk */
    gl_init();
    for (uint32_t i = 0; i < m.mod_init_count; i++) {
        uint32_t fn = macho_ptr(&m, m.mod_init_addr)[i];
        fprintf(stderr, "init %u/%u @ %08x\n", i + 1, m.mod_init_count, fn); fflush(stderr);
        guest_call2((void *)(uintptr_t)fn, 0, 0);
    }
    uint32_t mainp = LOAD_BASE + (GAME_MAIN_VM - m.vm_low) + (GAME_MAIN_THUMB ? 1 : 0);
    char *argv[] = {GAME_ARGV0, NULL};
    sys_start_watchdog();
    fprintf(stderr, "calling main @ %08x\n", mainp); fflush(stderr);
    int rc = guest_call2((void *)(uintptr_t)mainp, 1, (uintptr_t)argv);
    fprintf(stderr, "main returned %d\n", rc);
    return rc;
}
