/* OpenGL ES 1.1 (+OES extensions) -> vitaGL.  The guest is armv7 softfp, this module is hard-float:
 * every wrapper that takes a float is marked SOFT (aapcs, floats in core registers).  Guest pointers are real addresses
 * (same address space), so client-side vertex arrays and texture data can be handed to vitaGL as they are.
 * vitaGL symbols are declared weak so a missing one shows up as a log line instead of a link error. */
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <psp2/ctrl.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/io/fcntl.h>
#include <unistd.h>

#define SOFT __attribute__((pcs("aapcs")))
#define V(ret, name, args) extern ret name args __attribute__((weak))

V(void, vglInitExtended, (uint32_t, int, int, int, int));
V(unsigned, glCreateShader, (unsigned)); V(void, glShaderSource, (unsigned, int, const char *const *, const int *)); V(void, glCompileShader, (unsigned));
V(void, glGetShaderiv, (unsigned, unsigned, int *)); V(void, glGetShaderInfoLog, (unsigned, int, int *, char *)); V(unsigned, glCreateProgram, (void));
V(void, glAttachShader, (unsigned, unsigned)); V(void, glBindAttribLocation, (unsigned, unsigned, const char *)); V(void, glLinkProgram, (unsigned));
V(void, glGetProgramiv, (unsigned, unsigned, int *)); V(void, glGetProgramInfoLog, (unsigned, int, int *, char *)); V(void, glUseProgram, (unsigned));
V(int, glGetUniformLocation, (unsigned, const char *)); V(void, glUniformMatrix4fv, (int, int, unsigned char, const float *)); V(void, glUniform4fv, (int, int, const float *));
V(void, glUniform1f, (int, float)); V(void, glUniform1i, (int, int)); V(void, glVertexAttribPointer, (unsigned, int, unsigned, unsigned char, int, const void *));
V(void, glEnableVertexAttribArray, (unsigned)); V(void, glDisableVertexAttribArray, (unsigned)); V(void, vglSetParamBufferSize, (uint32_t));
V(void, vglUseVram, (unsigned char));
V(void, vglSwapBuffers, (unsigned char));
V(void, glActiveTexture, (unsigned));            V(void, glAlphaFunc, (unsigned, float));
V(void, glBindBuffer, (unsigned, unsigned));     V(void, glBindTexture, (unsigned, unsigned));
V(void, glBlendFunc, (unsigned, unsigned));      V(void, glBlendEquation, (unsigned));
V(void, glBufferData, (unsigned, int, const void *, unsigned)); V(void, glBufferSubData, (unsigned, int, int, const void *));
V(void, glClear, (unsigned));                    V(void, glClearColor, (float, float, float, float));
V(void, glClearDepthf, (float));                 V(void, glClearStencil, (int));
V(void, glClientActiveTexture, (unsigned));      V(void, glColor4ub, (unsigned char, unsigned char, unsigned char, unsigned char));
V(void, glColorMask, (unsigned char, unsigned char, unsigned char, unsigned char));
V(void, glColorPointer, (int, unsigned, int, const void *));
V(void, glCompressedTexImage2D, (unsigned, int, unsigned, int, int, int, int, const void *));
V(void, glCompressedTexSubImage2D, (unsigned, int, int, int, int, int, unsigned, int, const void *));
V(void, glCopyTexImage2D, (unsigned, int, unsigned, int, int, int, int, int));
V(void, glCullFace, (unsigned));                 V(void, glDeleteBuffers, (int, const unsigned *));
V(void, glDeleteFramebuffers, (int, const unsigned *)); V(void, glDeleteRenderbuffers, (int, const unsigned *));
V(void, glDeleteTextures, (int, const unsigned *)); V(void, glDepthFunc, (unsigned)); V(void, glDepthRangef, (float, float));
V(void, glDepthMask, (unsigned char));           V(void, glDisable, (unsigned));
V(void, glDisableClientState, (unsigned));       V(void, glDrawArrays, (unsigned, int, int));
V(void, glDrawElements, (unsigned, int, unsigned, const void *));
V(void, glEnable, (unsigned));                   V(void, glEnableClientState, (unsigned));
V(void, glFinish, (void));                       V(void, glFogf, (unsigned, float));
V(void, glFogfv, (unsigned, const float *));     V(void, glFramebufferRenderbuffer, (unsigned, unsigned, unsigned, unsigned));
V(void, glFramebufferTexture2D, (unsigned, unsigned, unsigned, unsigned, int));
V(void, glFrontFace, (unsigned));                V(void, glGenBuffers, (int, unsigned *));
V(void, glGenFramebuffers, (int, unsigned *));   V(void, glGenRenderbuffers, (int, unsigned *));
V(void, glGenTextures, (int, unsigned *));       V(void, glGenerateMipmap, (unsigned));
V(unsigned, glGetError, (void));                 V(void, glGetFloatv, (unsigned, float *));
V(void, glGetIntegerv, (unsigned, int *));       V(void, glLightModelfv, (unsigned, const float *));
V(void, glLightf, (unsigned, unsigned, float));  V(void, glLightfv, (unsigned, unsigned, const float *));
V(void, glLoadIdentity, (void));                 V(void, glLoadMatrixf, (const float *));
V(void, glMaterialf, (unsigned, unsigned, float)); V(void, glMaterialfv, (unsigned, unsigned, const float *));
V(void, glMatrixMode, (unsigned));               V(void, glMultMatrixf, (const float *));
V(void, glNormalPointer, (unsigned, int, const void *));
V(void, glReadPixels, (int, int, int, int, unsigned, unsigned, void *));
V(void, glBindFramebuffer, (unsigned, unsigned)); V(void, glBindRenderbuffer, (unsigned, unsigned));
V(void, glRenderbufferStorage, (unsigned, unsigned, int, int));
V(void, glScissor, (int, int, int, int));        V(void, glShadeModel, (unsigned));
V(void, glStencilFunc, (unsigned, int, unsigned)); V(void, glStencilMask, (unsigned));
V(void, glStencilOp, (unsigned, unsigned, unsigned));
V(void, glTexCoordPointer, (int, unsigned, int, const void *));
V(void, glFogi, (unsigned, int));
V(void, glTexEnvf, (unsigned, unsigned, float)); V(void, glTexImage2D, (unsigned, int, int, int, int, int, unsigned, unsigned, const void *));
V(void, glTexParameterf, (unsigned, unsigned, float)); V(void, glTexParameteri, (unsigned, unsigned, int));
V(void, glTexSubImage2D, (unsigned, int, int, int, int, int, unsigned, unsigned, const void *));
V(void, glPushMatrix, (void)); V(void, glPopMatrix, (void));
V(void, glPixelStorei, (unsigned, int)); V(void, glHint, (unsigned, unsigned)); V(void, glLightModelf, (unsigned, float));
V(void, glTexEnvi, (unsigned, unsigned, int)); V(void, glTexEnvfv, (unsigned, unsigned, const float *)); V(void, glLineWidth, (float));
V(void, glPointSize, (float)); V(void, glPolygonOffset, (float, float)); V(void, glColor4f, (float, float, float, float));
V(void, glNormal3f, (float, float, float)); V(void, glOrthof, (float, float, float, float, float, float)); V(void, glFlush, (void));
V(unsigned char, glIsEnabled, (unsigned)); V(unsigned char, glIsTexture, (unsigned)); V(void, glClipPlanef, (unsigned, const float *));
V(void, glMultiTexCoord4f, (unsigned, float, float, float, float));

V(void, glVertexPointer, (int, unsigned, int, const void *)); V(void, glViewport, (int, int, int, int));


/* weak no-op fallbacks: used only if vitaGL does not provide the function (a strong definition from libvitaGL.a wins) */
#define FB(name, ...) __attribute__((weak)) void name(__VA_ARGS__) { fprintf(stderr, "[gl] FALLBACK no-op: %s\n", #name); }
FB(glCopyTexImage2D, unsigned a, int b, unsigned c, int d, int e, int f, int g, int h)
FB(glCompressedTexSubImage2D, unsigned a, int b, int c, int d, int e, int f, unsigned g, int h, const void *i)
FB(glClearStencil, int s)
FB(glBlendEquation, unsigned m)
FB(glClientActiveTexture, unsigned t)
FB(glGenerateMipmap, unsigned t)
FB(glColorMask, unsigned char a, unsigned char b, unsigned char c, unsigned char d)
FB(glFinish, void)
FB(glColor4ub, unsigned char a, unsigned char b, unsigned char c, unsigned char d)
FB(glLightModelfv, unsigned p, const float *v)

#define SCREEN_W 960
#define SCREEN_H 544
/* the game renders for a 480x320 screen; letterbox that into the 960x544 display */
/* Screen geometry as the game sees it: UIKit "points" x content scale = backing pixels. Configurable via ux0:data/baroque/vita.cfg
 * (width=, height=, scale=). The backing buffer is fitted (aspect preserved) into the 960x544 display. */
/* The game paces itself with usleep() to ~20 fps (sleep = 50 ms - frame time). fps=N in vita.cfg shortens those sleeps on the
 * render thread so the cap becomes N (default 30). */
static pthread_t g_render_thr; static volatile int g_render_thr_set; static int g_fps_cap = 30; static int g_follow_active; static int g_first_person_gl(void) { extern int g_first_person; return g_first_person; } static void fog_resync(void); static float g_campull = 0.f, g_camraise = 0.f;   /* vita.cfg campull= / camraise=: move the 3D camera back / up (world units; the follow camera sits ~250 behind) */ static float g_touch_sx = -1.f;   /* vita.cfg touchscale= (default: 480/width) */ static float g_fov = 1.0f, g_fpfov = 90.f;   /* fpfov: first-person horizontal FOV in degrees (vita.cfg fpfov=) */   /* vita.cfg fov=: widen perspective projections by this factor (1 = the game's own) */ static unsigned g_tot_calls; volatile unsigned g_req_sleep_last, g_req_sleep_n; extern volatile unsigned g_msg_count, g_gtod_count; static unsigned long long g_pf_sleep; static unsigned g_pf_slpn; static int g_pace = 2;
static volatile int g_turbo;                                    /* START held: fast-forward (no frame sleeps, no drawing) for skipping slow cutscenes/loading */
static unsigned long long prof_now(void);
/* game clock (vita.cfg gameclock=0 turns it off). The game reads gettimeofday() and, when it sees more than a frame's
 * worth of time since its last frame, runs extra update ticks without drawing to catch up (worst frames showed 2-7
 * 'execute' calls per presented frame, update148). That extra work makes the next frame late too -> a steady tick.
 * On the render thread the clock it sees is capped at one frame period past the last presented frame, so it never
 * falls behind; when a frame is slow the game slows down for that frame instead of catching up (like the PS2 did). */
static int g_gameclock = 0; static long long gc_off, gc_vlast; static int gc_init;
static long long gc_virtual(long long real) {
    long long v = real - gc_off;
    if (!gc_init) { gc_init = 1; gc_vlast = v; }
    long long cap = gc_vlast + 1000000 / (g_fps_cap > 0 ? g_fps_cap : 30);
    if (v > cap) { gc_off += v - cap; v = cap; }
    return v;
}
long long gl_game_time(long long real) {
    if (!g_gameclock || !g_render_thr_set || !pthread_equal(pthread_self(), g_render_thr)) return real;
    return gc_virtual(real);
}
static void gc_present(void) {
    if (!g_gameclock) return;
    struct timeval tv; gettimeofday(&tv, NULL);
    gc_vlast = gc_virtual((long long)tv.tv_sec * 1000000 + tv.tv_usec);
}
void gl_note_sleep(unsigned long long us) { if (g_render_thr_set && pthread_equal(pthread_self(), g_render_thr)) { g_pf_sleep += us; g_pf_slpn++; } }
extern void pad_cfg_line(const char *line); extern void pad_init(void);
unsigned gl_adjust_sleep(unsigned us) {
    if (g_pace == 2 && !g_turbo) return us;   /* pace=2 (default): the game paces itself - its frame limiter sleeps out the rest of its 1/30 s budget */
    if (g_pace == 1 && g_render_thr_set && pthread_equal(pthread_self(), g_render_thr)) return 0;   /* pace=1: our own steady pacing at swap instead of the game's sleeps */
    if (g_turbo && g_render_thr_set && pthread_equal(pthread_self(), g_render_thr)) return 0;
    if (!g_render_thr_set || g_fps_cap <= 20 || !pthread_equal(pthread_self(), g_render_thr)) return us;
    unsigned cut = 1000000u / 20u - 1000000u / (unsigned)g_fps_cap;
    return us > cut ? us - cut : 0;
}
float g_pt_w = GAME_PT_W, g_pt_h = GAME_PT_H, g_content_scale = GAME_SCALE;   /* game.h; vita.cfg width=/height=/scale= override */
static float g_game_w = 480.f, g_game_h = 320.f, g_scale = 1.7f, g_xoff, g_yoff;
/* Portrait games, Vita held sideways (vita.cfg rotate=1|2, 0 = letterboxed upright): the game renders at its own
 * resolution into g_rot_fbo (it stands in for "the window"), and every present draws that texture turned 90 degrees.
 * rotate=1: the game's top is at the Vita's right edge (turn the Vita counter-clockwise, D-pad at the bottom);
 * rotate=2: the game's top is at the Vita's left edge (turn the Vita clockwise, D-pad at the top). */
int g_rotate = 1; static unsigned g_rot_fbo, g_rot_tex, g_rot_rb; static float g_rot_s, g_rot_x0, g_rot_w, g_rot_h;
static unsigned win_fb(void) { return g_rotate ? g_rot_fbo : 0; }
static void geom_recompute(void) {
    g_game_w = (float)(int)(g_pt_w * g_content_scale + 0.5f); g_game_h = (float)(int)(g_pt_h * g_content_scale + 0.5f);
    float sx = SCREEN_W / g_game_w, sy = SCREEN_H / g_game_h; g_scale = sx < sy ? sx : sy;
    g_xoff = (SCREEN_W - g_game_w * g_scale) * 0.5f; g_yoff = (SCREEN_H - g_game_h * g_scale) * 0.5f;
    if (g_rotate) {   /* game pixels go 1:1 into the off-screen target; the rotated picture: game height along the panel width */
        float rs_w = SCREEN_W / g_game_h, rs_h = SCREEN_H / g_game_w; g_rot_s = rs_w < rs_h ? rs_w : rs_h;
        g_rot_w = g_game_h * g_rot_s; g_rot_h = g_game_w * g_rot_s; g_rot_x0 = (SCREEN_W - g_rot_w) * 0.5f;
        g_scale = 1.f; g_xoff = g_yoff = 0.f;
    }
}
static int g_nolight, g_nofog, g_swpvrtc, g_pvrtc_var, g_normalize, g_fixfmt = 1, g_nocolormat, g_nospec, g_view;
static float g_bloom = 1.0f; static unsigned g_bound_tex;
/* ---- capture tools: shadow of the GL state the game sets, a one-frame per-draw dump (flag file) and a texture dumper ---- */
static unsigned g_pd[8];   /* update189: pull-back diagnostics */
int g_verbose;   /* vita.cfg verbose=1: diagnostics (thread sampler, per-call counters, [pull]/[sa]) */
static float g_near_k = 1.f; static int g_nearboost = 1;   /* update191: vita.cfg nearboost=0 turns it off */
static unsigned g_enbits, g_clbits, g_bs, g_bd, g_beq = 0x8006, g_dmask = 1, g_dfunc = 0x201, g_afunc = 0x207, g_cm = 0xF, g_tenv = 0x2100;
static float g_aref; static unsigned g_col[4] = {255, 255, 255, 255};
static int g_vp[3], g_np[3], g_cp[3], g_tp[3];                  /* size,type,stride of the vertex/normal/color/texcoord arrays */
static unsigned g_arr_buf, g_elem_buf;
#define BUFID(t) ((t) == 0x8892 ? g_arr_buf : (t) == 0x8893 ? g_elem_buf : 0u)
/* ---- CPU lighting (vitaGL ignores vertex colours for diffuse and takes alpha from a material uniform, so lit draws are lit here instead) ---- */
typedef struct { int size; unsigned type; int stride; const void *ptr; unsigned buf; } ArrP;
static ArrP g_ap[4];                                            /* 0 vertex, 1 normal, 2 colour, 3 texcoord: params + buffer binding at pointer-call time */
static int g_dbgmode = 0;                                            /* SELECT cycles draw-class debugging modes (see dbg_skip) */
static const char *dbg_names[8] = { "normal (all fixes)", "WITHOUT the depth fix (old look)", "skip effects that cross the camera plane", "state fix for DEPTH ONLY (update103 behaviour)", "FOG TEST: no fog anywhere", "FOG TEST: fog distances x3", "LIGHT TEST: character lighting off (vertex colours only)", "all effects hidden" };
static int g_clip_draws, g_clip_tin, g_clip_tout, g_clip_fail;
static int g_xing_n, g_xing_ok;   /* effect draws crossing the near plane / not, this frame (logged) */
static int g_fxa_why[8]; static int g_gpulight = 1; static unsigned g_gl_gpu_draws; static unsigned g_lw_jobs; static int g_lightmt_cfg = 1; static int g_batch = 1; static int g_glcheck; static int g_sfilter = 1; static unsigned g_pf_calls, g_pf_skips;   /* vita.cfg statefilter=0 disables redundant-state skipping */
/* ---- frame profiler: time per bucket, logged once a second as [prof] (per-frame averages in ms) ---- */
extern unsigned long long sceKernelGetProcessTimeWide(void) __attribute__((weak));
static unsigned long long prof_now(void) { if (sceKernelGetProcessTimeWide) return sceKernelGetProcessTimeWide(); struct timeval tv; gettimeofday(&tv, NULL); return (unsigned long long)tv.tv_sec * 1000000ull + (unsigned long long)tv.tv_usec; }
static unsigned long long g_pf_lloop, g_pf_worst, g_pf_wsnap[7], g_pf_wb[7]; static unsigned g_pf_h40, g_pf_h50; static unsigned long long g_pf_draw, g_pf_light, g_pf_upload, g_pf_tex, g_pf_swap, g_pf_present, g_pf_frame, g_pf_last; static unsigned g_pf_frames; static int g_charfog = 1;   /* vita.cfg charfog=1: fog on lit (character) draws as the game sets it */ static float g_fog_s = 0.f, g_fog_e = 1.f, g_fog_d = 1.f; static unsigned g_fog_m = 0x800, g_fog_draws_on; static int g_fogx;
static void pass_probe(void); static float g_mat_spec[4] = {0,0,0,1}, g_shin = 0.f, g_l_spec[4] = {1,1,1,1}; static unsigned g_spec_vtx; static unsigned g_act_unit, g_cli_unit, g_u_tex[4], g_u_en[4], g_u_tc[4], g_u_env[4] = {0x2100, 0x2100, 0x2100, 0x2100};   /* per texture unit state (capture) */ static unsigned g_cl_vtx, g_cl_sat, g_cl_dark;   /* cpu-lit vertices, how many clamp to white / are black */ static int g_debugkeys, g_texdiag;   /* texdiag=1: texture alpha scan + texture dumps (slow: ~80 ms per texture) */   /* vita.cfg debugkeys=1: SELECT debug modes, self-tests, element-buffer shadows */ static int g_depth_reapply_n;
/* vitaGL loses the GPU depth-test/depth-write setting when a framebuffer is bound (and the game only sets it once), so the game's draws never wrote depth.
   Re-send the game's current depth state after every framebuffer bind and clear. Mode 1 = without this fix. */
static void depth_reapply(void); static int g_probe_from = -1, g_dprobe_done; static void mk_reset(void); static int g_world_n;   /* opaque scene draws so far this frame (for the lighter-scene modes) */ static int g_parambuf = 32;   /* vita.cfg parambuf= (MB): GXM parameter buffer; too small = mid-frame partial renders that can lose the depth buffer */   /* fx_alpha outcome counters: 0 ok-array 1 ok-const 2 bad colour type 3 bad index type 4 no elem shadow 5 no colour shadow 6 alloc */
static float g_fxalpha = 1.0f;                                   /* vita.cfg fxalpha=: alpha multiplier for translucent no-depth-write effects in every mode */
static float g_fogc[4] = {0, 0, 0, 1}; static float g_clr[4]; static int g_sc[4]; static int g_gvport[4] = { 0, 0, 640, 960 };
static int g_fxcull = 0;                                         /* translucent no-depth-write effect meshes are drawn with back-face culling (vita.cfg fxcull=0 disables) */
static int g_cpulight = 1, g_cl_ok, g_cl_why[8];
static int g_reup = 0, g_reup_n, g_reup_bad;                       /* re-send a skinned vertex buffer at draw time if the game's memory has valid positions but what was uploaded has NaN (vita.cfg reupload=0 disables) */
static unsigned g_matmode = 0x1700;
static float g_mv[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};   /* tracked modelview (column-major) */
static float g_pj[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};   /* tracked projection (column-major) */
static float *cur_mat(void) { return g_matmode == 0x1700 ? g_mv : g_matmode == 0x1701 ? g_pj : NULL; }
static float g_l_amb[4] = {0,0,0,1}, g_l_dif[4] = {1,1,1,1}, g_l_pos[4] = {0,0,1,0}, g_l_att[3] = {1,0,0}, g_gl_amb[4] = {0.2f,0.2f,0.2f,1}, g_emis[4] = {0,0,0,1};
typedef struct { unsigned id; uint8_t *d; int size; int nbd, nsub; unsigned sub_bytes; const void *lastsrc; int lastoff, lastsz; } Shad;
static Shad g_sh[2048]; static int g_sh_n; static unsigned g_sh_total;
static Shad *shad_find(unsigned id) { static int last; if (!id) return NULL; if (last < g_sh_n && g_sh[last].id == id) return &g_sh[last];
    for (int i = 0; i < g_sh_n; i++) if (g_sh[i].id == id) { last = i; return &g_sh[i]; } return NULL; }
static Shad *shad_get(unsigned id, int total) {                 /* find or create a CPU copy of an array buffer (capped), grown to at least `total` bytes */
    if (!id || total < 0 || total > (1 << 20)) return NULL;
    Shad *sh = shad_find(id);
    if (!sh) { if (g_sh_n >= 2048 || g_sh_total + (unsigned)total > (24u << 20)) return NULL; sh = &g_sh[g_sh_n++]; sh->id = id; sh->d = NULL; sh->size = 0; sh->nbd = sh->nsub = 0; sh->sub_bytes = 0; sh->lastsrc = NULL; sh->lastoff = 0; sh->lastsz = 0; }
    if (sh->size < total) { uint8_t *n = realloc(sh->d, (size_t)total ? (size_t)total : 1); if (!n) return NULL; memset(n + sh->size, 0, (size_t)(total - sh->size)); g_sh_total += (unsigned)(total - sh->size); sh->d = n; sh->size = total; }
    return sh;
}
static int g_dd_on, g_dd_n; static FILE *g_dd_fp;
static int g_dumpn; static unsigned g_dumpbytes;
static int en_bit(unsigned cap) { switch (cap) { case 0xB44: return 0; case 0xBE2: return 1; case 0xB71: return 2; case 0xBC0: return 3; case 0xB50: return 4; case 0xB60: return 5;
    case 0xDE1: return 6; case 0xB57: return 7; case 0xB90: return 8; case 0xBA1: return 9; case 0x4000: return 10; case 0xC11: return 11; default: return -1; } }
static void en_track(unsigned cap, int v) { if (g_dd_on && g_dd_fp && (cap == 0xB71 || cap == 0xB90 || cap == 0xB60)) fprintf(g_dd_fp, "s %d %s 0x%x\n", g_dd_n, v ? "enable" : "disable", cap); int b = en_bit(cap); if (b >= 0) { if (v) g_enbits |= 1u << b; else g_enbits &= ~(1u << b); } }
static void cl_track(unsigned a, int v) { int b = a - 0x8074; if (b >= 0 && b < 8) { if (v) g_clbits |= 1u << b; else g_clbits &= ~(1u << b); } }
static void dump_file(const char *name, const void *d, size_t n) {
    char path[160]; snprintf(path, sizeof path, GAME_DIR "dump/%s", name);
    FILE *f = fopen(path, "wb"); if (!f) return; fwrite(d, 1, n, f); fclose(f);
    fprintf(stderr, "[dump] %s (%u bytes)\n", name, (unsigned)n); fflush(stderr);
}
static short g_ti_a[512], g_ti_w[512]; static unsigned char g_ti_f[512];   /* per-texture-id: mean alpha (-1 unknown), width, source (1 rgba8888, 2 converted 16-bit, 3 pvrtc) */
static void tinfo_set(unsigned id, int w, int fmt, const unsigned char *rgba, int npix) {
    if (!g_texdiag || id >= 512 || !rgba || npix <= 0) return;
    long sum = 0; int step = npix > 8192 ? npix / 8192 : 1, n = 0;
    for (int i = 0; i < npix; i += step) { sum += rgba[(size_t)i * 4 + 3]; n++; }
    g_ti_a[id] = (short)(sum / (n ? n : 1)); g_ti_w[id] = (short)w; g_ti_f[id] = (unsigned char)fmt;
}
static int g_dumpcat[3];
static void tex_dump(int cat, const char *kind, unsigned id, int w, int h, const void *d, size_t n) {
    static const int cap[3] = { 30, 20, 120 };
    if (!g_texdiag || !d || g_dumpcat[cat] >= cap[cat] || g_dumpbytes + n > 40u * 1024 * 1024 || w < 16 || n > 600000) return;
    g_dumpcat[cat]++;
    if (!g_dumpn) mkdir(GAME_DIR "dump", 0777);
    char nm[96]; snprintf(nm, sizeof nm, "t%03d_id%u_%dx%d.%s", g_dumpn, id, w, h, kind);
    g_dumpn++; g_dumpbytes += (unsigned)n; dump_file(nm, d, n);
}
static int g_cullopt, g_tracefrom; static unsigned g_cull_en, g_cull_mode = 0x405, g_front = 0x901, g_ch[2][2][2];
int pvrtc_decode(const unsigned char *src, int w, int h, int bpp2, int has_alpha, int variant, unsigned char *dst);
static void geom_load_cfg(void) {
    FILE *f = fopen(GAME_DIR "vita.cfg", "r");
    if (f) {
        char line[128];
        while (fgets(line, sizeof line, f)) {
            pad_cfg_line(line); { extern void extra_cfg_line(const char *); extra_cfg_line(line); }
            float v; int iv; if (sscanf(line, "rotate=%d", &iv) == 1 && iv >= 0 && iv <= 2) g_rotate = iv;
            if (sscanf(line, "width=%f", &v) == 1 && v >= 160 && v <= 2048) g_pt_w = v;
            else if (sscanf(line, "height=%f", &v) == 1 && v >= 120 && v <= 1536) g_pt_h = v;
            else if (sscanf(line, "scale=%f", &v) == 1 && v >= 0.5f && v <= 4.f) g_content_scale = v;
            else if (sscanf(line, "pace=%f", &v) == 1) g_pace = (int)v;
            else if (sscanf(line, "fps=%f", &v) == 1 && v >= 10 && v <= 60) g_fps_cap = (int)v;
            else if (sscanf(line, "nolight=%f", &v) == 1) g_nolight = (int)v;
            else if (sscanf(line, "cpulight=%f", &v) == 1) g_cpulight = (int)v;
            else if (sscanf(line, "reupload=%f", &v) == 1) g_reup = (int)v;
            else if (sscanf(line, "fxcull=%f", &v) == 1) g_fxcull = (int)v;
            else if (sscanf(line, "debugkeys=%f", &v) == 1) g_debugkeys = (int)v;
            else if (sscanf(line, "texdiag=%f", &v) == 1) g_texdiag = (int)v;
            else if (sscanf(line, "fov=%f", &v) == 1) g_fov = v;
            else if (sscanf(line, "fpfov=%f", &v) == 1) g_fpfov = v;
            else if (sscanf(line, "gameclock=%f", &v) == 1) g_gameclock = (int)v;
            else if (sscanf(line, "workercore=%f", &v) == 1) { extern int g_worker_core; g_worker_core = (int)v; }
            else if (sscanf(line, "maincore=%f", &v) == 1) { extern int g_main_core; g_main_core = (int)v; }
            else if (sscanf(line, "campull=%f", &v) == 1) g_campull = v;
            else if (sscanf(line, "nearboost=%f", &v) == 1) g_nearboost = (int)v;
            else if (sscanf(line, "camclip=%f", &v) == 1) { extern int g_clip_on; g_clip_on = (int)v; }
            else if (sscanf(line, "verbose=%f", &v) == 1) g_verbose = (int)v;
            else if (sscanf(line, "camreal=%f", &v) == 1) { extern int g_camreal; g_camreal = (int)v; }
            else if (sscanf(line, "camraise=%f", &v) == 1) g_camraise = v;
            else if (sscanf(line, "touchscale=%f", &v) == 1) g_touch_sx = v;
            else if (sscanf(line, "glcheck=%f", &v) == 1) g_glcheck = (int)v;
            else if (sscanf(line, "statefilter=%f", &v) == 1) g_sfilter = (int)v;
            else if (sscanf(line, "batch=%f", &v) == 1) g_batch = (int)v;
            else if (sscanf(line, "gpulight=%f", &v) == 1) g_gpulight = (int)v;
            else if (sscanf(line, "lightmt=%f", &v) == 1) g_lightmt_cfg = (int)v;
            else if (sscanf(line, "charfog=%f", &v) == 1) g_charfog = (int)v;
            else if (sscanf(line, "parambuf=%f", &v) == 1 && v >= 0 && v <= 96) g_parambuf = (int)v;
            else if (sscanf(line, "fxalpha=%f", &v) == 1 && v >= 0.f && v <= 1.f) g_fxalpha = v;
            else if (sscanf(line, "fxmode=%f", &v) == 1 && v >= 0 && v < 8) g_dbgmode = (int)v;
            else if (sscanf(line, "nofog=%f", &v) == 1) g_nofog = (int)v;
            else if (sscanf(line, "swpvrtc=%f", &v) == 1) g_swpvrtc = (int)v;
            else if (sscanf(line, "pvrtcvar=%f", &v) == 1) g_pvrtc_var = (int)v;
            else if (sscanf(line, "normalize=%f", &v) == 1) g_normalize = (int)v;
            else if (sscanf(line, "fixfmt=%f", &v) == 1) g_fixfmt = (int)v;
            else if (sscanf(line, "nocolormat=%f", &v) == 1) g_nocolormat = (int)v;
            else if (sscanf(line, "nospec=%f", &v) == 1) g_nospec = (int)v;
            else if (sscanf(line, "view=%f", &v) == 1) g_view = (int)v;
            else if (sscanf(line, "bloom=%f", &v) == 1 && v >= 0.f && v <= 4.f) g_bloom = v;
            else if (sscanf(line, "cull=%f", &v) == 1) g_cullopt = (int)v;
            else if (sscanf(line, "tracefrom=%f", &v) == 1) g_tracefrom = (int)v;
        }
        fclose(f);
    }
    geom_recompute();
    pad_init();
    fprintf(stderr, "[gl] fps cap %d (game default 20), pacing %s\n", g_fps_cap, g_pace == 2 ? "the game's own 30 fps limiter (pace=2)" : g_pace ? "steady (pace=1)" : "game's own sleeps, shortened (pace=0)");
    fprintf(stderr, "[gl] switches: nolight=%d nofog=%d swpvrtc=%d pvrtcvar=%d normalize=%d fixfmt=%d nocolormat=%d nospec=%d view=%d bloom=%.2f cull=%d cpulight=%d fxcull=%d fxalpha=%.2f fxmode=%d depthrange=%d tracefrom=%d\n", g_nolight, g_nofog, g_swpvrtc, g_pvrtc_var, g_normalize, g_fixfmt, g_nocolormat, g_nospec, g_view, (double)g_bloom, g_cullopt, g_cpulight, g_fxcull, (double)g_fxalpha, g_dbgmode, glDepthRangef != NULL, g_tracefrom);
    fprintf(stderr, "[gl] screen: %.0fx%.0f points, scale %.2f -> backing %.0fx%.0f, fit x%.3f offset %.0f,%.0f%s\n", g_pt_w, g_pt_h, g_content_scale,
            g_game_w, g_game_h, g_scale, g_xoff, g_yoff, f ? " (vita.cfg)" : " (defaults)");
    fflush(stderr);
}
float gl_screen_points_w(void) { return g_pt_w; }
float gl_screen_points_h(void) { return g_pt_h; }
float gl_screen_scale(void) { return g_content_scale; }

/* Widescreen: the game draws its HUD for the real width (568) but hit-tests touches in a 480-wide space, so touch x is
 * scaled by 480/width before the game sees it (measured in update135: taps on the drawn icons at x~550 missed). */
static float touch_sx(void) { return g_touch_sx > 0.f ? g_touch_sx : 1.f;   /* Baroque needed 480/width; BB3 hit-tests in its own points */ }
float gl_touch_sx(void) { return touch_sx(); }
void gl_game_to_screen(float gx, float gy, float *nx, float *ny) {   /* UIKit points -> normalised panel coords (inverse of gl_screen_to_game) */
    gx /= touch_sx();
    if (g_rotate) {   /* points -> panel: game y runs along the panel width, game x along the panel height */
        float u = gx / g_pt_w, v = gy / g_pt_h, sx, sy;
        if (g_rotate == 1) { sx = g_rot_x0 + (1.f - v) * g_rot_w; sy = u * SCREEN_H; }
        else               { sx = g_rot_x0 + v * g_rot_w;         sy = (1.f - u) * SCREEN_H; }
        *nx = sx / SCREEN_W; *ny = sy / SCREEN_H; return;
    }
    *nx = (gx * g_content_scale * g_scale + g_xoff) / SCREEN_W; *ny = (gy * g_content_scale * g_scale + g_yoff) / SCREEN_H;
}
void gl_screen_to_game(float nx, float ny, float *gx, float *gy) {   /* normalised panel coords (0..1) -> UIKit points */
    float sx = nx * SCREEN_W, sy = ny * SCREEN_H;
    if (g_rotate) {
        float a = (sx - g_rot_x0) / g_rot_w, b = sy / SCREEN_H, u, v;   /* a: across the picture's long side, b: down the panel */
        if (a < 0) a = 0; if (a > 1) a = 1; if (b < 0) b = 0; if (b > 1) b = 1;
        if (g_rotate == 1) { v = 1.f - a; u = b; } else { v = a; u = 1.f - b; }
        *gx = u * g_pt_w * touch_sx(); *gy = v * g_pt_h; return;
    }
    float x = (sx - g_xoff) / g_scale / g_content_scale, y = (sy - g_yoff) / g_scale / g_content_scale;
    if (x < 0) x = 0; if (x > g_pt_w) x = g_pt_w; if (y < 0) y = 0; if (y > g_pt_h) y = g_pt_h;
    *gx = x * touch_sx(); *gy = y;
}

static const char *g_last = "?";
static unsigned g_draws, g_frames;
static unsigned g_prev_draws; static FILE *g_snap_fp; static int g_snap_from;
static unsigned g_cur_rb, g_cur_fb;
static unsigned g_default_fb_bound = 1;           /* 1 while the window framebuffer (fb 0) is bound */

static void first(const char *n, const void *fn) {
    static const char *seen[160]; static int ns;
    for (int i = 0; i < ns; i++) if (seen[i] == n) return;
    if (ns < 160) seen[ns++] = n;
    fprintf(stderr, "[gl] first call: %s%s\n", n, fn ? "" : "  (NOT AVAILABLE in vitaGL -> ignored)"); fflush(stderr);
}
/* ---- draw batching: consecutive glDrawElements (ushort strips/triangles from the same buffers) with no other GL call in between are merged into one
   triangle list; ANY other GL call flushes first, so order and state are exactly the game's (vita.cfg batch=0 disables) ---- */
static uint16_t *g_bq; static int g_bq_n, g_bq_cap; static unsigned g_pf_hw, g_pf_batched;
static const char g_name_de[] = "glDrawElements";
static void batch_flush(void);
/* gap finder: every 16th GL call, note the time since the previous mark; the biggest gap of each frame (and the GL calls on
 * either side) is kept for the worst frame of the second -> shows where a slow frame spends its time */
static unsigned long long g_gap_t, g_fgap, g_wgap; static const char *g_gap_name = "<start>", *g_fgap_a, *g_fgap_b, *g_wgap_a, *g_wgap_b;
static unsigned g_gap_fcalls, g_fgap_idx, g_wgap_idx; static const char *g_wsel;
static unsigned long long prof_now(void);
unsigned long long gl_prof_now(void) { return prof_now(); }
/* async log: lines are formatted into memory and written to the memory card by a low-priority thread on core 0.
 * Any write to ux0 blocks the caller 20-80 ms; done on the game thread once a second it was the remaining tick (update171). */
static char g_plog_buf[2][32768]; static int g_plog_len, g_plog_cur; static pthread_mutex_t g_plog_mx = PTHREAD_MUTEX_INITIALIZER; static int g_plog_started;
static void *plog_writer(void *arg) {
    (void)arg;
    extern int sceKernelChangeThreadCpuAffinityMask(int thid, int mask) __attribute__((weak));
    extern int sceKernelChangeThreadPriority(int thid, int prio) __attribute__((weak));
    if (sceKernelChangeThreadCpuAffinityMask) sceKernelChangeThreadCpuAffinityMask(0, 0x10000);
    if (sceKernelChangeThreadPriority) sceKernelChangeThreadPriority(0, 190);
    for (;;) {
        usleep(250000);
        pthread_mutex_lock(&g_plog_mx); int n = g_plog_len, b = g_plog_cur; g_plog_cur ^= 1; g_plog_len = 0; pthread_mutex_unlock(&g_plog_mx);
        if (n > 0) { fwrite(g_plog_buf[b], 1, (size_t)n, stderr); fflush(stderr); }
    }
    return NULL;
}
void plog(const char *fmt, ...) {
    if (!g_plog_started) { g_plog_started = 1; pthread_t th; pthread_attr_t a; pthread_attr_init(&a); pthread_attr_setstacksize(&a, 0x4000); pthread_create(&th, &a, plog_writer, NULL); }
    char line[1024]; va_list ap; va_start(ap, fmt); int n = vsnprintf(line, sizeof line, fmt, ap); va_end(ap);
    if (n <= 0) return; if (n >= (int)sizeof line) n = sizeof line - 1;
    pthread_mutex_lock(&g_plog_mx);
    if (g_plog_len + n <= (int)sizeof g_plog_buf[0]) { memcpy(g_plog_buf[g_plog_cur] + g_plog_len, line, (size_t)n); g_plog_len += n; }
    pthread_mutex_unlock(&g_plog_mx);
}
int gl_fps_cap(void) { return g_fps_cap; }
/* execute bookkeeping: time spent in 'execute' calls that did not present a frame (update without drawing) */
volatile unsigned g_present_count; static unsigned long long g_px_cur; static unsigned long long g_nx_us, g_nx_max; static unsigned g_nx_n, g_wnx_n; static unsigned long long g_wnx_us, g_wnx_max, g_px_last;
/* sync waits on the render (presenting) thread: 0 sched_yield, 1 mutex blocked, 2 semaphore blocked, 3 failed sem_trywait */
static unsigned long long g_sy_us[4], g_wsy_us[4]; static unsigned g_sy_n[4], g_wsy_n[4];
void gl_note_sync(int kind, unsigned long long us) { if (kind < 0 || kind > 3 || !g_render_thr_set || !pthread_equal(pthread_self(), g_render_thr)) return; g_sy_us[kind] += us; g_sy_n[kind]++; }
static unsigned g_wws[4];
static unsigned long long g_ph_at, g_ph_upd, g_ph_ours, g_wph_at, g_wph_upd, g_wph_ours, g_last_present_t;
/* engine thread CPU time: how much of a frame the presenting thread actually ran (vs. was preempted / blocked) */
typedef SceKernelThreadInfo BqThreadInfo;
static unsigned long long g_run_last, g_frame_run, g_wframe_run; static unsigned g_preempt_last, g_frame_preempt, g_wframe_preempt, g_cpu_now, g_wcpu;
static void run_sample(void) {
    if (!g_verbose) return;   /* update214 cleanup: diagnostics off unless vita.cfg verbose=1 */
    BqThreadInfo ti; memset(&ti, 0, sizeof ti); ti.size = sizeof ti;
    if (sceKernelGetThreadInfo(0, &ti) < 0) return;
    unsigned pre = ti.intrPreemptCount + ti.threadPreemptCount;
    if (g_run_last) { g_frame_run = ti.runClocks - g_run_last; g_frame_preempt = pre - g_preempt_last; }
    g_run_last = ti.runClocks; g_preempt_last = pre; g_cpu_now = (unsigned)ti.lastExecutedCpuId;
}
void gl_note_padhook(unsigned long long t, unsigned long long upd, unsigned long long ours) { g_ph_at = g_last_present_t ? t - g_last_present_t : 0; g_ph_upd = upd; g_ph_ours = ours; }
static unsigned long long g_poll_us, g_poll_max, g_wpoll_us, g_wpoll_max; static unsigned g_poll_n, g_wpoll_n;
void gl_note_poll(unsigned long long us) { g_poll_us += us; g_poll_n++; if (us > g_poll_max) g_poll_max = us; }
void gl_note_exec(unsigned long long us, int presented) { if (presented) g_px_cur = us; else { g_nx_n++; g_nx_us += us; if (us > g_nx_max) g_nx_max = us; } }
static void gap_mark(const char *n) {
    unsigned long long t = prof_now();
    if (g_gap_t && t - g_gap_t > g_fgap) { g_fgap = t - g_gap_t; g_fgap_a = g_gap_name; g_fgap_b = n; g_fgap_idx = g_gap_fcalls; }
    g_gap_t = t; g_gap_name = n;
}
/* per-call timing for the first 48 GL calls of each frame (frame start = scene setup, where a GPU wait would show) */
static unsigned long long g_fc_t, g_fc_max, g_wfc_max; static const char *g_fc_name = "<after present>", *g_fc_maxname, *g_wfc_maxname; static unsigned g_fc_maxidx, g_wfc_maxidx;
static void fc_mark(const char *n) {
    unsigned long long t = prof_now();
    if (g_fc_t && t - g_fc_t > g_fc_max) { g_fc_max = t - g_fc_t; g_fc_maxname = g_fc_name; g_fc_maxidx = g_gap_fcalls; }
    g_fc_t = t; g_fc_name = n;
}
#define ENTER(n, fn) do { if (g_bq_n && (const char *)(n) != g_name_de) batch_flush(); g_last = n; g_pf_calls++; g_tot_calls++; if (g_gap_fcalls < 48) fc_mark(n); if (!(++g_gap_fcalls & 15)) gap_mark(n); static int s_first_; if (!s_first_) { s_first_ = 1; first(n, (const void *)(fn)); } } while (0)   /* per call site: no list scan per call */
/* ---- diagnostics: every distinct GL state/format combination is logged once ([u] lines); [perf] line once a second ---- */
static unsigned g_ul_h[2048]; static int g_ul_n;
static void ulog(const char *fmt, ...) {
    char b[200]; va_list ap; va_start(ap, fmt); vsnprintf(b, sizeof b, fmt, ap); va_end(ap);
    unsigned h = 2166136261u; for (const char *q = b; *q; q++) h = (h ^ (unsigned char)*q) * 16777619u; if (!h) h = 1;
    unsigned i = h & 2047; while (g_ul_h[i]) { if (g_ul_h[i] == h) return; i = (i + 1) & 2047; }
    g_ul_h[i] = h; g_ul_n++; fprintf(stderr, "[u] %s\n", b);
}
static int g_diag = 1;   /* per-call diagnostics (unique-state log, glGetError after every call): first ~10 s only, or vita.cfg glcheck=1 */
#define ULOG(...) do { if (g_diag && g_ul_n < 900) ulog(__VA_ARGS__); } while (0)
static int g_vl_n;
#define VLOG(...) do { if (g_vl_n < 160) { g_vl_n++; fprintf(stderr, "[v] " __VA_ARGS__); fputc(10, stderr); fflush(stderr); } } while (0)
static unsigned g_st_texup, g_st_texpix, g_st_fbbind, g_st_draws;
static unsigned g_a, g_b; static int g_nerr_call;
static void gl_chk(const char *n) {
    if (!g_diag || !glGetError) return;
    unsigned e = glGetError();
    if (e) { g_nerr_call++; ULOG("ERROR 0x%x in %s (args 0x%x 0x%x)", e, n, g_a, g_b); }
}
#define ARGS(a, b) do { g_a = (unsigned)(a); g_b = (unsigned)(b); } while (0)

static int g_trace;
static unsigned g_win_drawn, g_last_draw_fb;
static unsigned g_fbt_fb[32], g_fbt_tex[32]; static int g_fbt_n;
static void fbt_set(unsigned fb, unsigned tex) {
    for (int i = 0; i < g_fbt_n; i++) if (g_fbt_fb[i] == fb) { g_fbt_tex[i] = tex; return; }
    if (g_fbt_n < 32) { g_fbt_fb[g_fbt_n] = fb; g_fbt_tex[g_fbt_n++] = tex; }
}
static unsigned fbt_get(unsigned fb) { for (int i = 0; i < g_fbt_n; i++) if (g_fbt_fb[i] == fb) return g_fbt_tex[i]; return 0; }
static void dd_record(const char *fn, unsigned mode, int count, unsigned type) {
    if (!g_dd_on || !g_dd_fp) return;
    fprintf(g_dd_fp, "%d %s mode=0x%x n=%d type=0x%x | fbtex=%u tex=%u | en=0x%03x cl=0x%02x | blend %x,%x eq=%x | depth mask=%u func=%x | alpha %x %.3f | cmask=%x | tenv=%x | col=%u,%u,%u,%u | V(%d,%x,%d) N(%d,%x,%d) C(%d,%x,%d) T(%d,%x,%d) | arrbuf=%u | ta=%d tw=%d tf=%d\n",
            g_dd_n++, fn, mode, count, type, fbt_get(g_cur_fb), g_bound_tex, g_enbits, g_clbits, g_bs, g_bd, g_beq, g_dmask, g_dfunc, g_afunc, (double)g_aref, g_cm, g_tenv,
            g_col[0], g_col[1], g_col[2], g_col[3], g_vp[0], g_vp[1], g_vp[2], g_np[0], g_np[1], g_np[2], g_cp[0], g_cp[1], g_cp[2], g_tp[0], g_tp[1], g_tp[2], g_arr_buf, g_bound_tex < 512 && g_ti_f[g_bound_tex] ? g_ti_a[g_bound_tex] : -1, g_bound_tex < 512 ? g_ti_w[g_bound_tex] : 0, g_bound_tex < 512 ? g_ti_f[g_bound_tex] : 0);
    fprintf(g_dd_fp, "u %d act=%u cli=%u | u0 en=%u tex=%u tc=%u env=%x | u1 en=%u tex=%u tc=%u env=%x | u2 en=%u tex=%u\n", g_dd_n - 1, g_act_unit, g_cli_unit, g_u_en[0], g_u_tex[0], g_u_tc[0], g_u_env[0], g_u_en[1], g_u_tex[1], g_u_tc[1], g_u_env[1], g_u_en[2], g_u_tex[2]);
}
static void dd_geom(int is_arrays, int first, int count, unsigned itype, const void *ip) {   /* second dump line: where the draw lands on screen (NDC bbox, depth range) */
    if (!g_dd_on || !g_dd_fp) return;
    int id = g_dd_n - 1;
    if (g_ap[0].type != 0x1406 || g_ap[0].size < 3) { fprintf(g_dd_fp, "g %d novtx\n", id); return; }
    int vst = g_ap[0].stride ? g_ap[0].stride : 12, vmax = 1 << 20; const uint8_t *vb;
    if (g_ap[0].buf) { Shad *sh = shad_find(g_ap[0].buf); if (!sh) { fprintf(g_dd_fp, "g %d noshadow\n", id); return; }
        size_t off = (size_t)(uintptr_t)g_ap[0].ptr; vb = sh->d + off; vmax = (int)(((size_t)sh->size > off ? (size_t)sh->size - off : 0) / (size_t)vst); }
    else vb = (const uint8_t *)g_ap[0].ptr;
    if (!is_arrays && (uintptr_t)ip < 0x10000) {   /* indices live in a bound element buffer: use its CPU copy */
        Shad *es = g_elem_buf ? shad_find(g_elem_buf) : NULL; size_t isz = itype == 0x1401 ? 1 : itype == 0x1405 ? 4 : 2;
        if (!es || (size_t)(uintptr_t)ip + (size_t)count * isz > (size_t)es->size) { fprintf(g_dd_fp, "g %d elembuf-noshadow\n", id); return; }
        ip = es->d + (uintptr_t)ip; }
    const uint8_t *cbp = NULL; int cst = g_ap[2].stride ? g_ap[2].stride : 4, amin = 999, amax = -1; int rmin = 999, rmax = -1, gmin = 999, gmax = -1, bmin = 999, bmax = -1; long rs = 0, gs = 0, bs = 0, cn = 0;
    if ((g_clbits & 4) && g_ap[2].type == 0x1401 && g_ap[2].size == 4) { if (g_ap[2].buf) { Shad *cs = shad_find(g_ap[2].buf); if (cs) cbp = cs->d + (size_t)(uintptr_t)g_ap[2].ptr; } else cbp = (const uint8_t *)g_ap[2].ptr; }
    int n = count > 6000 ? 6000 : count; float xa = 1e9f, xb = -1e9f, ya = 1e9f, yb = -1e9f, za = 1e9f, zb = -1e9f; int in = 0, behind = 0, bad = 0, tot = 0;
    for (int k = 0; k < n; k++) {
        int vi; if (is_arrays) vi = first + k; else vi = itype == 0x1401 ? ((const uint8_t *)ip)[k] : itype == 0x1405 ? (int)((const uint32_t *)ip)[k] : ((const uint16_t *)ip)[k];
        if (vi < 0 || vi >= vmax) { bad++; continue; }
        if (cbp) { const uint8_t *cc = cbp + (size_t)vi * (size_t)cst; int al = cc[3]; if (al < amin) amin = al; if (al > amax) amax = al;
            if (cc[0] < rmin) rmin = cc[0]; if (cc[0] > rmax) rmax = cc[0]; if (cc[1] < gmin) gmin = cc[1]; if (cc[1] > gmax) gmax = cc[1]; if (cc[2] < bmin) bmin = cc[2]; if (cc[2] > bmax) bmax = cc[2]; rs += cc[0]; gs += cc[1]; bs += cc[2]; cn++; }
        const float *v = (const float *)(vb + (size_t)vi * (size_t)vst); float e[4];
        for (int r = 0; r < 4; r++) e[r] = g_mv[r] * v[0] + g_mv[4 + r] * v[1] + g_mv[8 + r] * v[2] + g_mv[12 + r];
        float c[4]; for (int r = 0; r < 4; r++) c[r] = g_pj[r] * e[0] + g_pj[4 + r] * e[1] + g_pj[8 + r] * e[2] + g_pj[12 + r] * e[3];
        tot++; if (c[3] <= 1e-6f) { behind++; continue; }
        float x = c[0] / c[3], y = c[1] / c[3], z = c[2] / c[3];
        if (x < xa) xa = x; if (x > xb) xb = x; if (y < ya) ya = y; if (y > yb) yb = y; if (z < za) za = z; if (z > zb) zb = z;
        if (x >= -1 && x <= 1 && y >= -1 && y <= 1 && z >= -1 && z <= 1) in++;
    }
    if (tot > 0 && xa > 1e8f) {   /* nothing projected to a finite position: say whether the matrices or the vertex data are broken */
        int nfv = 0, nz = 0; const float *v0 = NULL;
        for (int k = 0; k < n; k++) { int vi = is_arrays ? first + k : itype == 0x1401 ? ((const uint8_t *)ip)[k] : itype == 0x1405 ? (int)((const uint32_t *)ip)[k] : ((const uint16_t *)ip)[k];
            if (vi < 0 || vi >= vmax) continue; const float *v = (const float *)(vb + (size_t)vi * (size_t)vst); if (!v0) v0 = v;
            if (!isfinite(v[0]) || !isfinite(v[1]) || !isfinite(v[2])) nfv++; else if (v[0] == 0.f && v[1] == 0.f && v[2] == 0.f) nz++; }
        int mvok = 1, pjok = 1; for (int k = 0; k < 16; k++) { if (!isfinite(g_mv[k])) mvok = 0; if (!isfinite(g_pj[k])) pjok = 0; }
        { Shad *sb = g_ap[0].buf ? shad_find(g_ap[0].buf) : NULL; const uint32_t *w = (const uint32_t *)vb;
          fprintf(g_dd_fp, "gb %d buf=%u size=%d bufferdata_calls=%d subdata_calls=%d sub_bytes=%u lastsrc=%p lastsuboff=%d first_words=%08x %08x %08x %08x %08x %08x stride=%d off=%u\n", id, g_ap[0].buf, sb ? sb->size : -1, sb ? sb->nbd : -1, sb ? sb->nsub : -1, sb ? sb->sub_bytes : 0, sb ? sb->lastsrc : NULL, sb ? sb->lastoff : -1, w[0], w[1], w[2], w[3], w[4], w[5], vst, (unsigned)(uintptr_t)g_ap[0].ptr); }
        fprintf(g_dd_fp, "gn %d mv_finite=%d pj_finite=%d vtx_nonfinite=%d vtx_zero=%d/%d v0=(%g %g %g) mv=[%g %g %g %g / %g %g %g %g / %g %g %g %g / %g %g %g %g]\n", id, mvok, pjok, nfv, nz, tot,
                v0 ? (double)v0[0] : 0., v0 ? (double)v0[1] : 0., v0 ? (double)v0[2] : 0., (double)g_mv[0], (double)g_mv[4], (double)g_mv[8], (double)g_mv[12], (double)g_mv[1], (double)g_mv[5], (double)g_mv[9], (double)g_mv[13], (double)g_mv[2], (double)g_mv[6], (double)g_mv[10], (double)g_mv[14], (double)g_mv[3], (double)g_mv[7], (double)g_mv[11], (double)g_mv[15]);
    }
    fprintf(g_dd_fp, "g %d x[%.2f,%.2f] y[%.2f,%.2f] z[%.4f,%.4f] in=%d/%d behind=%d bad=%d alpha[%d,%d] rgb r[%d,%d] g[%d,%d] b[%d,%d] mean=%ld,%ld,%ld\n", id, (double)xa, (double)xb, (double)ya, (double)yb, (double)za, (double)zb, in, tot, behind, bad, amin == 999 ? -1 : amin, amax, rmin, rmax, gmin, gmax, bmin, bmax, cn ? rs / cn : -1, cn ? gs / cn : -1, cn ? bs / cn : -1);
}
/* capture helper: save the 8 MB of host heap around the most-updated skinned vertex buffer (64 KB chunks that are mapped; the file is records of [addr][len][bytes]) */
static __attribute__((unused)) void dump_heap_window(void) {
    const void *c = NULL; int best = 0;
    for (int i = 0; i < g_sh_n; i++) if (g_sh[i].nsub > best && g_sh[i].lastsrc) { best = g_sh[i].nsub; c = g_sh[i].lastsrc; }
    if (!c) { fprintf(stderr, "[dump] heap window: no skinned buffer seen\n"); fflush(stderr); return; }
    uintptr_t base = ((uintptr_t)c & ~(uintptr_t)0xFFFF) - (4u << 20), end = base + (8u << 20);
    SceUID fd = sceIoOpen(GAME_DIR "heap.bin", SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777), pf = sceIoOpen(GAME_DIR "probe.tmp", SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
    if (fd < 0 || pf < 0) { if (fd >= 0) sceIoClose(fd); if (pf >= 0) sceIoClose(pf); return; }
    int ok = 0, bad = 0;
    for (uintptr_t a = base; a < end; a += 0x10000) {
        sceIoLseek(pf, 0, SCE_SEEK_SET);
        if (sceIoWrite(pf, (const void *)a, 0x10000) != 0x10000) { bad++; continue; }
        uint32_t hdr[2] = { (uint32_t)a, 0x10000 }; sceIoWrite(fd, hdr, 8); sceIoWrite(fd, (const void *)a, 0x10000); ok++;
    }
    sceIoClose(fd); sceIoClose(pf); remove(GAME_DIR "probe.tmp");
    fprintf(stderr, "[dump] heap window around %p: %d chunks saved, %d unmapped\n", c, ok, bad); fflush(stderr);
}
static void note_draw(void) { if (g_default_fb_bound) g_win_drawn = 1; else g_last_draw_fb = g_cur_fb; }
static int g_fbev;
#define FBLOG(...) do { if (g_fbev < 80) { g_fbev++; fprintf(stderr, "[fb] " __VA_ARGS__); fputc('\n', stderr); fflush(stderr); } } while (0)
#define CALL(fn, ...) do { if (g_trace) fprintf(stderr, "[t] %s a=%x b=%x win=%u\n", g_last, g_a, g_b, g_default_fb_bound); \
                           if (fn) { fn(__VA_ARGS__); gl_chk(g_last); } } while (0)

/* ---------------- window framebuffer = the "drawable" renderbuffer ---------------- */
static unsigned g_drawable_rb;       /* renderbuffer name that was given storage from the CAEAGLLayer */
static int g_rb_w[512], g_rb_h[512];
static unsigned g_default_fbs[16]; static int g_n_default_fbs;
static unsigned g_skip_rbs[16];    static int g_n_skip_rbs;     /* depth/stencil buffers attached to the window framebuffer */
static int in_list(unsigned *l, int n, unsigned v) { for (int i = 0; i < n; i++) if (l[i] == v) return 1; return 0; }

static __attribute__((used)) void gl_mark_drawable_impl(void) {
    /* the game attaches the colour renderbuffer to its fb BEFORE giving it storage, so the fb that is bound right now is "the window" */
    g_drawable_rb = g_cur_rb;
    if (g_cur_fb && !in_list(g_default_fbs, g_n_default_fbs, g_cur_fb) && g_n_default_fbs < 16) g_default_fbs[g_n_default_fbs++] = g_cur_fb;
    g_default_fb_bound = 1;
    CALL(glBindFramebuffer, 0x8D40, win_fb());
    fprintf(stderr, "[gl] drawable renderbuffer = 0x%x, window framebuffer = 0x%x\n", g_cur_rb, g_cur_fb); fflush(stderr);
}

void gl_init(void) {
    if (!vglInitExtended) { fprintf(stderr, "[gl] vitaGL not linked!\n"); return; }
    if (vglUseVram) vglUseVram(1);
    geom_load_cfg();
    if (g_parambuf > 0 && vglSetParamBufferSize) vglSetParamBufferSize((uint32_t)g_parambuf * 1024u * 1024u);
    fprintf(stderr, "[gl] parameter buffer: %s %d MB\n", vglSetParamBufferSize ? "set to" : "NOT SETTABLE (vitaGL lacks vglSetParamBufferSize), wanted", g_parambuf); fflush(stderr);
    vglInitExtended(0, SCREEN_W, SCREEN_H, 0x1800000, 0);
    fprintf(stderr, "[gl] vitaGL initialised (%dx%d)\n", SCREEN_W, SCREEN_H); fflush(stderr);
    if (g_rotate) {
        int w = (int)g_game_w, h = (int)g_game_h;
        glGenTextures(1, &g_rot_tex); glBindTexture(0x0DE1, g_rot_tex);
        glTexImage2D(0x0DE1, 0, 0x1908, w, h, 0, 0x1908, 0x1401, NULL);
        glTexParameteri(0x0DE1, 0x2801, 0x2601); glTexParameteri(0x0DE1, 0x2800, 0x2601);   /* linear: the picture is scaled to 85% */
        glTexParameteri(0x0DE1, 0x2802, 0x812F); glTexParameteri(0x0DE1, 0x2803, 0x812F);
        glGenFramebuffers(1, &g_rot_fbo); glBindFramebuffer(0x8D40, g_rot_fbo);
        glFramebufferTexture2D(0x8D40, 0x8CE0, 0x0DE1, g_rot_tex, 0);
        glGenRenderbuffers(1, &g_rot_rb); glBindRenderbuffer(0x8D41, g_rot_rb);
        glRenderbufferStorage(0x8D41, 0x88F0 /* DEPTH24_STENCIL8 */, w, h);
        glFramebufferRenderbuffer(0x8D40, 0x8D00, 0x8D41, g_rot_rb); glFramebufferRenderbuffer(0x8D40, 0x8D20, 0x8D41, g_rot_rb);
        glBindTexture(0x0DE1, 0);
        fprintf(stderr, "[gl] rotate=%d: game renders %dx%d off-screen (fbo %u), shown turned 90 degrees at %.0fx%.0f, x offset %.0f\n",
                g_rotate, w, h, g_rot_fbo, (double)g_rot_w, (double)g_rot_h, (double)g_rot_x0); fflush(stderr);
    }
}


/* The game renders its final image into an off-screen texture and (as far as the log shows) never draws to the window itself.
 * If nothing was drawn to the window this frame, show the last off-screen target, letterboxed. */
/* Put back every piece of GL state present_fallback changed. It used to leave the texture env on GL_REPLACE (the game sets MODULATE only once at start-up),
   which made every textured draw ignore vertex colours, CPU lighting, vertex alpha and the bloom weight. Mode 1 (game as-is) keeps the old behaviour for comparison. */
static void present_restore(void) {
    if (g_dbgmode == 1) return;
    CALL(glMatrixMode, g_matmode);
    CALL(glActiveTexture, 0x84C1); if (g_u_en[1]) CALL(glEnable, 0x0DE1); else CALL(glDisable, 0x0DE1); CALL(glTexEnvf, 0x2300, 0x2200, (float)g_u_env[1]); CALL(glBindTexture, 0x0DE1, g_u_tex[1]);
    CALL(glActiveTexture, 0x84C0); if (g_u_en[0]) CALL(glEnable, 0x0DE1); else CALL(glDisable, 0x0DE1); CALL(glTexEnvf, 0x2300, 0x2200, (float)g_u_env[0]); CALL(glBindTexture, 0x0DE1, g_u_tex[0]);
    CALL(glActiveTexture, 0x84C0 + g_act_unit);
    CALL(glClientActiveTexture, 0x84C1); if (g_u_tc[1]) CALL(glEnableClientState, 0x8078); else CALL(glDisableClientState, 0x8078);
    CALL(glClientActiveTexture, 0x84C0); if (g_u_tc[0]) CALL(glEnableClientState, 0x8078); else CALL(glDisableClientState, 0x8078);
    if (g_clbits & 1) CALL(glEnableClientState, 0x8074); else CALL(glDisableClientState, 0x8074);
    if (g_clbits & 2) CALL(glEnableClientState, 0x8075); else CALL(glDisableClientState, 0x8075);
    if (g_clbits & 4) CALL(glEnableClientState, 0x8076); else CALL(glDisableClientState, 0x8076);
    if (g_ap[0].ptr || g_ap[0].buf) { CALL(glBindBuffer, 0x8892, g_ap[0].buf); CALL(glVertexPointer, g_ap[0].size, g_ap[0].type, g_ap[0].stride, g_ap[0].ptr); }
    CALL(glClientActiveTexture, 0x84C0 + g_cli_unit);
    CALL(glBindBuffer, 0x8892, g_arr_buf);
    static const unsigned caps[11] = { 0xB44, 0xBE2, 0xB71, 0xBC0, 0xB50, 0xB60, 0xB57, 0xB90, 0x0BA1, 0x4000, 0xC11 }; static const int bits[11] = { 0, 1, 2, 3, 4, 5, 7, 8, 9, 10, 11 };
    for (int i = 0; i < 11; i++) { if (caps[i] == 0xB60 && g_nofog) continue; if ((g_enbits >> bits[i]) & 1) CALL(glEnable, caps[i]); else CALL(glDisable, caps[i]); }
    CALL(glColor4ub, g_col[0], g_col[1], g_col[2], g_col[3]); CALL(glClearColor, g_clr[0], g_clr[1], g_clr[2], g_clr[3]);
}
static void present_fallback(unsigned tex) {
    static const float quad[8] = { -1, -1, 1, -1, -1, 1, 1, 1 };
    static const float uv[8]   = {  0,  0, 1,  0,  0, 1, 1, 1 };
    static int once; if (!once) { once = 1; fprintf(stderr, "[gl] present fallback: showing texture %u (fb 0x%x)\n", tex, g_last_draw_fb); fflush(stderr); }
    CALL(glBindFramebuffer, 0x8D40, win_fb());
    CALL(glDisable, 0x0C11); CALL(glDisable, 0x0B71); CALL(glDisable, 0x0B44); CALL(glDisable, 0x0BE2);
    CALL(glDisable, 0x0B50); CALL(glDisable, 0x0B60); CALL(glDisable, 0x0BC0); CALL(glDisable, 0x0B90);
    CALL(glViewport, 0, 0, SCREEN_W, SCREEN_H);
    CALL(glClearColor, 0.f, 0.f, 0.f, 1.f);
    CALL(glClear, 0x4100);
    CALL(glViewport, (int)g_xoff, (int)g_yoff, (int)(g_game_w * g_scale), (int)(g_game_h * g_scale));
    CALL(glBindBuffer, 0x8892, 0);
    CALL(glActiveTexture, 0x84C1); CALL(glDisable, 0x0DE1);
    CALL(glClientActiveTexture, 0x84C1); CALL(glDisableClientState, 0x8078);
    CALL(glActiveTexture, 0x84C0); CALL(glEnable, 0x0DE1); CALL(glBindTexture, 0x0DE1, tex);
    CALL(glTexEnvf, 0x2300, 0x2200, 7681.f /* GL_REPLACE */);
    CALL(glClientActiveTexture, 0x84C0);
    CALL(glEnableClientState, 0x8074); CALL(glEnableClientState, 0x8078);
    CALL(glDisableClientState, 0x8076); CALL(glDisableClientState, 0x8075);
    CALL(glMatrixMode, 0x1701); CALL(glPushMatrix); CALL(glLoadIdentity);
    CALL(glMatrixMode, 0x1700); CALL(glPushMatrix); CALL(glLoadIdentity);
    CALL(glMatrixMode, 0x1702); CALL(glPushMatrix); CALL(glLoadIdentity);
    CALL(glVertexPointer, 2, 0x1406, 0, quad);
    CALL(glTexCoordPointer, 2, 0x1406, 0, uv);
    CALL(glColor4ub, 255, 255, 255, 255);
    CALL(glDrawArrays, 5, 0, 4);
    CALL(glMatrixMode, 0x1702); CALL(glPopMatrix);
    CALL(glMatrixMode, 0x1700); CALL(glPopMatrix);
    CALL(glMatrixMode, 0x1701); CALL(glPopMatrix);
    present_restore();
}


/* rotate mode: draw the off-screen game picture onto the panel turned 90 degrees */
static void rot_present(void) {
    static const float uv1[8] = { 1, 0,  1, 1,  0, 0,  0, 1 }, uv2[8] = { 0, 1,  0, 0,  1, 1,  1, 0 };
    float x0 = -1.f + 2.f * g_rot_x0 / SCREEN_W, x1 = -x0;
    float quad[8] = { x0, -1,  x1, -1,  x0, 1,  x1, 1 };
    glBindFramebuffer(0x8D40, 0);
    CALL(glDisable, 0x0C11); CALL(glDisable, 0x0B71); CALL(glDisable, 0x0B44); CALL(glDisable, 0x0BE2);
    CALL(glDisable, 0x0B50); CALL(glDisable, 0x0B60); CALL(glDisable, 0x0BC0); CALL(glDisable, 0x0B90);
    CALL(glColorMask, 1, 1, 1, 1);
    CALL(glViewport, 0, 0, SCREEN_W, SCREEN_H);
    CALL(glClearColor, 0.f, 0.f, 0.f, 1.f); CALL(glClear, 0x4000);
    CALL(glBindBuffer, 0x8892, 0);
    CALL(glActiveTexture, 0x84C1); CALL(glDisable, 0x0DE1);
    CALL(glClientActiveTexture, 0x84C1); CALL(glDisableClientState, 0x8078);
    CALL(glActiveTexture, 0x84C0); CALL(glEnable, 0x0DE1); CALL(glBindTexture, 0x0DE1, g_rot_tex);
    CALL(glTexEnvf, 0x2300, 0x2200, 7681.f /* GL_REPLACE */);
    CALL(glClientActiveTexture, 0x84C0);
    CALL(glEnableClientState, 0x8074); CALL(glEnableClientState, 0x8078);
    CALL(glDisableClientState, 0x8076); CALL(glDisableClientState, 0x8075);
    CALL(glMatrixMode, 0x1701); CALL(glPushMatrix); CALL(glLoadIdentity);
    CALL(glMatrixMode, 0x1700); CALL(glPushMatrix); CALL(glLoadIdentity);
    CALL(glMatrixMode, 0x1702); CALL(glPushMatrix); CALL(glLoadIdentity);
    CALL(glVertexPointer, 2, 0x1406, 0, quad);
    CALL(glTexCoordPointer, 2, 0x1406, 0, g_rotate == 1 ? uv1 : uv2);
    CALL(glColor4ub, 255, 255, 255, 255);
    CALL(glDrawArrays, 5, 0, 4);
    CALL(glMatrixMode, 0x1702); CALL(glPopMatrix);
    CALL(glMatrixMode, 0x1700); CALL(glPopMatrix);
    CALL(glMatrixMode, 0x1701); CALL(glPopMatrix);
    present_restore();
    CALL(glColorMask, g_cm & 1, (g_cm >> 1) & 1, (g_cm >> 2) & 1, (g_cm >> 3) & 1);
}
static void draw_mode_indicator(void); static void draw_pointer(void); static void color_selftest(const char *why); static unsigned *g_cw0, *g_csec, g_cnow0, g_cnow1, g_cnow2, g_cnow3;
static __attribute__((used)) void gl_present_impl(void) {
    batch_flush();
    gc_present();
    if (g_pace == 1 && !g_turbo && g_fps_cap > 0) {   /* steady pacing: hold each frame to 1/fps measured from a running deadline */
        static unsigned long long next; unsigned long long per = 1000000ull / (unsigned)g_fps_cap, t = prof_now();
        if (!next || t > next + per / 2) next = t + per;
        else { if (next > t + 1000) { usleep((unsigned)(next - t)); g_pf_sleep += prof_now() - t; g_pf_slpn++; } next += per; }
    }
    { unsigned long long t = prof_now(); unsigned long long ft = g_pf_last ? t - g_pf_last : 0; if (g_pf_last) g_pf_frame += ft; g_pf_last = t; g_pf_frames++;
      unsigned long long cur[7] = { g_pf_draw, g_pf_light, g_pf_upload, g_pf_tex, g_pf_swap, (unsigned long long)g_st_texup, g_pf_sleep };
      static unsigned c_prev[4], c_w[4], c_sec[4]; static int c_init; unsigned c_now[4] = { g_tot_calls, g_msg_count, g_gtod_count, g_req_sleep_n };
      if (!c_init) { c_init = 1; for (int k = 0; k < 4; k++) c_prev[k] = c_sec[k] = c_now[k]; }
      gap_mark("<present>"); g_present_count++; run_sample();
      { extern volatile unsigned g_follow_tick; static unsigned last; g_follow_active = (g_follow_tick != last); last = g_follow_tick; fog_resync();
        static unsigned pf; if (g_verbose && ++pf % 30 == 0) { extern int g_view_diag; extern float g_pull_scale;
          plog("[pull] per 30 frames: no-follow %u fp %u not-ok %u | hud-skip depthoff %u depthon %u | shifted depthoff %u depthon %u | diag %d scale %d%%",
               g_pd[0], g_pd[1], g_pd[2], g_pd[3], g_pd[4], g_pd[5], g_pd[6], g_view_diag, (int)(g_pull_scale * 100.f)); memset(g_pd, 0, sizeof g_pd); } }
      { extern const char *g_msg_ring[8]; static char wsel[200]; unsigned nm = c_now[1] - c_prev[1];
        if (ft > g_pf_worst) { int o = 0; wsel[0] = 0; for (unsigned k = 0; k < nm && k < 8; k++) o += snprintf(wsel + o, sizeof wsel - o, " %s", g_msg_ring[(c_now[1] - 1 - k) & 7] ? g_msg_ring[(c_now[1] - 1 - k) & 7] : "?"); g_wsel = wsel; } }
      if (ft > g_pf_worst) { g_wnx_n = g_nx_n; g_wnx_us = g_nx_us; g_wnx_max = g_nx_max; g_wpoll_us = g_poll_us; g_wpoll_max = g_poll_max; g_wpoll_n = g_poll_n; g_px_last = g_px_cur; g_wph_at = g_ph_at; g_wph_upd = g_ph_upd; g_wph_ours = g_ph_ours; g_wframe_run = g_frame_run; g_wframe_preempt = g_frame_preempt; g_wcpu = g_cpu_now; g_wfc_max = g_fc_max; g_wfc_maxname = g_fc_maxname; g_wfc_maxidx = g_fc_maxidx; { extern volatile unsigned g_ws_frame[4]; for (int k = 0; k < 4; k++) g_wws[k] = g_ws_frame[k]; } for (int k = 0; k < 4; k++) { g_wsy_us[k] = g_sy_us[k]; g_wsy_n[k] = g_sy_n[k]; } }
      for (int k = 0; k < 4; k++) { g_sy_us[k] = 0; g_sy_n[k] = 0; }
      g_nx_n = 0; g_nx_us = 0; g_nx_max = 0; g_poll_us = 0; g_poll_max = 0; g_poll_n = 0;
      if (ft > g_pf_worst) { for (int k = 0; k < 4; k++) c_w[k] = c_now[k] - c_prev[k]; g_wgap = g_fgap; g_wgap_a = g_fgap_a; g_wgap_b = g_fgap_b; g_wgap_idx = g_fgap_idx; }
      { extern volatile unsigned g_ws_frame[4]; for (int k = 0; k < 4; k++) g_ws_frame[k] = 0; }
      g_fgap = 0; g_gap_fcalls = 0; g_gap_name = "<after present>"; g_last_present_t = prof_now(); g_fc_max = 0; g_fc_t = g_last_present_t; g_fc_name = "<after present>";
      for (int k = 0; k < 4; k++) c_prev[k] = c_now[k];
      g_cw0 = c_w; g_csec = c_sec; g_cnow0 = c_now[0]; g_cnow1 = c_now[1]; g_cnow2 = c_now[2]; g_cnow3 = c_now[3];
      if (ft > 40000) g_pf_h40++; if (ft > 50000) g_pf_h50++;
      if (ft > g_pf_worst) { g_pf_worst = ft; for (int k = 0; k < 7; k++) g_pf_wb[k] = cur[k] - g_pf_wsnap[k]; }
      for (int k = 0; k < 7; k++) g_pf_wsnap[k] = cur[k]; }
    if (g_diag && !g_glcheck && g_frames > 300) { g_diag = 0; fprintf(stderr, "[gl] per-call diagnostics off from frame %u (vita.cfg glcheck=1 keeps them)\n", g_frames); fflush(stderr); }
    if (!g_render_thr_set) { g_render_thr = pthread_self(); g_render_thr_set = 1; fprintf(stderr, "[gl] frames are presented by thread %p\n", (void *)(uintptr_t)pthread_self()); fflush(stderr); }
    g_frames++; g_trace = g_tracefrom > 0 ? (g_frames >= (unsigned)g_tracefrom && g_frames <= (unsigned)g_tracefrom + 2) : 0;   /* boot trace off (tracefrom=N in vita.cfg) */
    if (g_trace) { fprintf(stderr, "[t] ===== frame %u =====\n", g_frames); fflush(stderr); }
    if (glGetError) {
        unsigned e = glGetError();
        static int nerr;
        if (e && nerr < 30) { nerr++; plog("[gl] ERROR 0x%x (last call %s)\n", e, g_last); }
    }
    if (g_frames == 1 || g_frames % 300 == 0) { plog("[gl] frame %u, draws in last frame %u\n", g_frames, g_draws); }
    g_prev_draws = g_draws; g_draws = 0; g_world_n = 0;
    { static struct timeval t0; static unsigned f0; struct timeval t1; gettimeofday(&t1, NULL);
      if (!t0.tv_sec) { t0 = t1; f0 = g_frames; }
      double dt = (t1.tv_sec - t0.tv_sec) + (t1.tv_usec - t0.tv_usec) * 1e-6;
      if (dt >= 1.0) { unsigned nf = g_frames - f0 ? g_frames - f0 : 1;
          plog("[perf] fps=%.1f draws/frame=%u texUploads/frame=%u texMpix/frame=%.2f fbBinds/frame=%u errors=%d cullDraws[off=%u back/ccw=%u back/cw=%u front/ccw=%u front/cw=%u]\n",
                  nf / dt, g_st_draws / nf, g_st_texup / nf, g_st_texpix / 1e6 / nf, g_st_fbbind / nf, g_nerr_call,
                  g_ch[0][0][0] + g_ch[0][0][1] + g_ch[0][1][0] + g_ch[0][1][1], g_ch[1][0][0], g_ch[1][0][1], g_ch[1][1][0], g_ch[1][1][1]);
          if (g_pf_frames) { double f = 1000.0 * g_pf_frames; double fr = g_pf_frame / f, dr = g_pf_draw / f, li = g_pf_light / f, up = g_pf_upload / f, tx = g_pf_tex / f, sw = g_pf_swap / f;
              plog("[prof] GL calls/frame=%u (skipped as redundant=%u) | draws sent to vitaGL/frame=%u (game draws merged=%u) | per frame (ms): total=%.1f | draw calls=%.1f (of which cpu lighting=%.1f) | buffer uploads=%.1f | texture uploads=%.1f | waiting for GPU at swap=%.1f | sleeping=%.1f (%u sleeps) | rest (game logic + all other GL calls)=%.1f | frames over 40 ms=%u, over 50 ms=%u\n",
                      g_pf_calls / g_pf_frames, g_pf_skips / g_pf_frames, g_pf_hw / g_pf_frames, g_pf_batched / g_pf_frames, fr, dr, li, up, tx, sw, g_pf_sleep / f, g_pf_slpn, fr - dr - up - tx - sw - g_pf_sleep / f, g_pf_h40, g_pf_h50);
              g_pf_calls = g_pf_skips = g_pf_hw = g_pf_batched = 0;
              plog("[prof] lit draws on GPU=%u/s | lighting maths=%.1f ms/frame (split jobs=%u) | worst frame this second=%.1f ms: draws=%.1f lighting=%.1f buffer uploads=%.1f texture uploads=%.1f (%llu textures) swap=%.1f sleeping=%.1f rest=%.1f\n",
                      g_gl_gpu_draws, g_pf_lloop / f, g_lw_jobs, g_pf_worst / 1000.0, g_pf_wb[0] / 1000.0, g_pf_wb[1] / 1000.0, g_pf_wb[2] / 1000.0, g_pf_wb[3] / 1000.0, g_pf_wb[5], g_pf_wb[4] / 1000.0, g_pf_wb[6] / 1000.0,
                      (g_pf_worst - g_pf_wb[0] - g_pf_wb[2] - g_pf_wb[3] - g_pf_wb[4] - g_pf_wb[6]) / 1000.0);
              if (g_cw0 && g_csec) { unsigned nf = g_pf_frames ? g_pf_frames : 1;
                  plog("[prof] per frame avg: GL calls=%u objc msgs=%u time reads=%u game sleeps=%u (last asked %u us) | worst frame: GL calls=%u objc msgs=%u time reads=%u game sleeps=%u\n",
                      (g_cnow0 - g_csec[0]) / nf, (g_cnow1 - g_csec[1]) / nf, (g_cnow2 - g_csec[2]) / nf, (g_cnow3 - g_csec[3]) / nf, g_req_sleep_last, g_cw0[0], g_cw0[1], g_cw0[2], g_cw0[3]);
                  if (g_wsel) plog("[prof] worst frame's objc messages:%s\n", g_wsel);
                  plog("[prof] worst frame: presenting thread actually ran %.1f ms of it, preempted %u times, on cpu %d\n", g_wframe_run / 1000.0, g_wframe_preempt, (int)g_wcpu);
                  plog("[prof] worst frame, engine thread samples (2 ms each): running %u, sleeping %u, waiting on type-0x10 object %u, other waits %u\n", g_wws[0], g_wws[1], g_wws[2], g_wws[3]);
                  if (g_wfc_maxname) plog("[prof] worst frame: slowest step among the first 48 GL calls: %.1f ms after %s (call %u)\n", g_wfc_max / 1000.0, g_wfc_maxname, g_wfc_maxidx);
                  plog("[prof] worst frame: game pad update started %.1f ms after the previous present, took %.1f ms (+ %.1f ms ours)\n", g_wph_at / 1000.0, g_wph_upd / 1000.0, g_wph_ours / 1000.0);
                  plog("[prof] worst frame, presenting thread waits: sched_yield %u x %.1f ms, mutex %u x %.1f ms, semaphore %u x %.1f ms, failed trywait %u\n", g_wsy_n[0], g_wsy_us[0] / 1000.0, g_wsy_n[1], g_wsy_us[1] / 1000.0, g_wsy_n[2], g_wsy_us[2] / 1000.0, g_wsy_n[3]);
                  plog("[prof] worst frame: %u execute calls without drawing took %.1f ms (longest %.1f ms); last drawing execute %.1f ms; input polling %u times %.1f ms (longest %.1f)\n", g_wnx_n, g_wnx_us / 1000.0, g_wnx_max / 1000.0, g_px_last / 1000.0, g_wpoll_n, g_wpoll_us / 1000.0, g_wpoll_max / 1000.0);
                  if (g_wgap_a) plog("[prof] worst frame's biggest gap: %.1f ms between %s and %s (around GL call %u of the frame)\n", g_wgap / 1000.0, g_wgap_a, g_wgap_b, g_wgap_idx);
                  g_csec[0] = g_cnow0; g_csec[1] = g_cnow1; g_csec[2] = g_cnow2; g_csec[3] = g_cnow3; }
              g_pf_lloop = 0; g_pf_worst = 0; g_lw_jobs = 0; g_gl_gpu_draws = 0;
              g_pf_draw = g_pf_light = g_pf_upload = g_pf_tex = g_pf_swap = g_pf_frame = 0; g_pf_frames = 0; for (int k = 0; k < 5; k++) g_pf_wsnap[k] = 0; g_pf_wsnap[5] = g_st_texup; g_pf_sleep = 0; g_pf_wsnap[6] = 0; g_pf_slpn = g_pf_h40 = g_pf_h50 = 0; }
          { extern void snd_stats(char *out, int n); static int sn; if (++sn % 5 == 0) { char sb[160]; snd_stats(sb, sizeof sb); plog("%s\n", sb); } }
          if (g_cpulight && g_cl_vtx) { plog("[light] light0 amb=%.2f,%.2f,%.2f dif=%.2f,%.2f,%.2f global_amb=%.2f,%.2f,%.2f emis=%.2f pos=%.2f,%.2f,%.2f,%.0f | mat_spec=%.2f,%.2f,%.2f shininess=%.1f light_spec=%.2f draws_with_specular=%u | fog mode=0x%x start=%.1f end=%.1f density=%.3f | lit vertices=%u clamped_to_white=%u%% black=%u%%\n",
              (double)g_l_amb[0], (double)g_l_amb[1], (double)g_l_amb[2], (double)g_l_dif[0], (double)g_l_dif[1], (double)g_l_dif[2], (double)g_gl_amb[0], (double)g_gl_amb[1], (double)g_gl_amb[2], (double)g_emis[0],
              (double)g_l_pos[0], (double)g_l_pos[1], (double)g_l_pos[2], (double)g_l_pos[3], (double)g_mat_spec[0], (double)g_mat_spec[1], (double)g_mat_spec[2], (double)g_shin, (double)g_l_spec[0], g_spec_vtx, g_fog_m, (double)g_fog_s, (double)g_fog_e, (double)g_fog_d, g_cl_vtx, g_cl_sat * 100u / g_cl_vtx, g_cl_dark * 100u / g_cl_vtx); g_cl_vtx = g_cl_sat = g_cl_dark = 0; g_spec_vtx = 0; }
          if (g_cpulight) plog("[cpulight] ok=%d fail(en/type/tex/data)=%d/%d/%d/%d shadows=%d/%uKB reupload=%d bad=%d fxalpha[okarr=%d okconst=%d coltype=%d idxtype=%d noelem=%d nocol=%d] coltype=0x%x/%d depthfix=%d crossing=%d notcrossing=%d clip[draws=%d tris_in=%d tris_out=%d fail=%d]\n", g_cl_ok, g_cl_why[1], g_cl_why[2], g_cl_why[3], g_cl_why[4], g_sh_n, g_sh_total >> 10, g_reup_n, g_reup_bad, g_fxa_why[0], g_fxa_why[1], g_fxa_why[2], g_fxa_why[3], g_fxa_why[4], g_fxa_why[5], g_ap[2].type, g_ap[2].size, g_depth_reapply_n, g_xing_n, g_xing_ok, g_clip_draws, g_clip_tin, g_clip_tout, g_clip_fail); g_xing_n = g_xing_ok = 0; g_clip_draws = g_clip_tin = g_clip_tout = g_clip_fail = 0;
          memset(g_ch, 0, sizeof g_ch);
          g_st_draws = g_st_texup = g_st_texpix = g_st_fbbind = 0; t0 = t1; f0 = g_frames; { extern void imp_prof_report(void); imp_prof_report(); } } }
    if (g_dd_on) { g_dd_on = 0; if (g_dd_fp) { fclose(g_dd_fp); g_dd_fp = NULL; } if (g_snap_fp) { fclose(g_snap_fp); g_snap_fp = NULL; } fprintf(stderr, "[dump] draw dump written: %d draws\n", g_dd_n); fflush(stderr); }
    else {
        static int prev_lr; SceCtrlData pad; int trig = 0;   /* hold L+R on the Vita to capture the next frame's draw calls */
        if (sceCtrlPeekBufferPositive(0, &pad, 1) > 0) { int lr = g_debugkeys && (pad.buttons & SCE_CTRL_LTRIGGER) && (pad.buttons & SCE_CTRL_RTRIGGER) && (pad.buttons & SCE_CTRL_SELECT);   /* capture: L+R+SELECT, debugkeys only (L/R are game buttons now) */ static int burst, held; if (lr) { held++; if ((held == 1 || held % 15 == 0) && burst < 12) { trig = 1; burst++; } } else { held = 0; burst = 0; } prev_lr = lr; { int t = g_debugkeys && (pad.buttons & SCE_CTRL_START) && (pad.buttons & SCE_CTRL_LTRIGGER); if (t != g_turbo) { g_turbo = t; fprintf(stderr, "[turbo] %s\n", t ? "ON" : "off"); fflush(stderr); } } static int prev_sel; int sel = g_debugkeys && (pad.buttons & SCE_CTRL_SELECT) && (pad.buttons & SCE_CTRL_RTRIGGER) && !(pad.buttons & SCE_CTRL_LTRIGGER); if (sel && !prev_sel) { g_dbgmode = (g_dbgmode + 1) % 8; fprintf(stderr, "[dbg] mode %d (%s)\n", g_dbgmode, dbg_names[g_dbgmode]); fflush(stderr); } prev_sel = sel; }
        if (0 && !trig && g_frames % 15 == 0) {   /* debug only: a file check on the memory card blocks the game thread 20-70 ms (this was the twice-a-second tick) */ FILE *ff = fopen(GAME_DIR "dumpdraw.flag", "r"); if (ff) { fclose(ff); remove(GAME_DIR "dumpdraw.flag"); trig = 1; } }
        static int dump_k; if (trig) { char dn[64]; snprintf(dn, sizeof dn, GAME_DIR "dump_%02d.txt", dump_k % 12); dump_k++; g_dd_fp = fopen(dn, "w"); g_dd_n = 0; g_snap_from = g_prev_draws > 260 ? (int)g_prev_draws - 260 : 0; g_snap_fp = NULL; if (g_dd_fp) { g_dd_on = 1; g_probe_from = -1; g_dprobe_done = 0; mk_reset(); fprintf(g_dd_fp, "mode %d (%s)\n", g_dbgmode, dbg_names[g_dbgmode]); fprintf(stderr, "[dump] capturing next frame\n"); fflush(stderr); } }
    }
    if (g_turbo && (g_frames & 7)) { g_win_drawn = 0; g_last_draw_fb = 0; return; }
    if (g_view > 0) present_fallback((unsigned)g_view);   /* debug: show off-screen texture N instead of the final image */
    else if (!g_win_drawn && g_last_draw_fb) { unsigned tex = fbt_get(g_last_draw_fb); if (tex) present_fallback(tex); }
    g_win_drawn = 0; g_last_draw_fb = 0;
    if (g_turbo && (g_frames & 7)) return;                       /* fast-forward: show only every 8th frame */
    if (g_debugkeys && g_frames == 600) color_selftest(g_frames == 600 ? "frame 600" : "frame 2400");
    draw_mode_indicator(); draw_pointer();
    if (g_rotate && g_rot_fbo) rot_present();
    { unsigned long long t0 = prof_now(); if (vglSwapBuffers) vglSwapBuffers(0); g_pf_swap += prof_now() - t0; }
    if (g_rotate && g_rot_fbo) {   /* back to the game's target, with its viewport/scissor as it left them */
        if (g_default_fb_bound) glBindFramebuffer(0x8D40, g_rot_fbo); else if (g_cur_fb) glBindFramebuffer(0x8D40, g_cur_fb);
        glViewport(g_gvport[0], g_gvport[1], g_gvport[2], g_gvport[3]); glScissor(g_sc[0], g_sc[1], g_sc[2], g_sc[3]);
    }
}

/* ---------------- state ---------------- */
static __attribute__((used)) void w_glActiveTexture(unsigned t) { ENTER("glActiveTexture", glActiveTexture); ARGS(t,0); g_act_unit = (t - 0x84C0) & 3; CALL(glActiveTexture, t); }
static __attribute__((used)) void SOFT w_glAlphaFunc(unsigned f, float r) { ENTER("glAlphaFunc", glAlphaFunc); if (g_sfilter && f == g_afunc && r == g_aref) { g_pf_skips++; return; } g_afunc = f; g_aref = r; ULOG("alphaFunc 0x%x ref=%d", f, (int)(r*100)); ARGS(f,0); CALL(glAlphaFunc, f, r); }
static __attribute__((used)) void w_glBlendFunc(unsigned s, unsigned d) { ENTER("glBlendFunc", glBlendFunc); if (g_sfilter && s == g_bs && d == g_bd) { g_pf_skips++; return; } g_bs = s; g_bd = d; ULOG("blendFunc 0x%x 0x%x", s, d); ARGS(s,d); CALL(glBlendFunc, s, d); }
static __attribute__((used)) void w_glBlendEquationOES(unsigned m) { ENTER("glBlendEquationOES", glBlendEquation); g_beq = m; ULOG("blendEq 0x%x", m); ARGS(m,0); CALL(glBlendEquation, m); }
static __attribute__((used)) void SOFT w_glClearColor(float r, float g, float b, float a) { ENTER("glClearColor", glClearColor); g_clr[0] = r; g_clr[1] = g; g_clr[2] = b; g_clr[3] = a; CALL(glClearColor, r, g, b, a); }
static __attribute__((used)) void SOFT w_glClearDepthf(float d) { ENTER("glClearDepthf", glClearDepthf); CALL(glClearDepthf, d); }
static __attribute__((used)) void w_glClearStencil(int s) { ENTER("glClearStencil", glClearStencil); CALL(glClearStencil, s); }
static __attribute__((used)) void w_glClear(unsigned m) { ENTER("glClear", glClear); { extern void vreal_apply(void); vreal_apply(); } if (g_dd_on && g_dd_fp) fprintf(g_dd_fp, "s %d clear 0x%x fb=%u\n", g_dd_n, m, fbt_get(g_cur_fb)); ARGS(m,0); note_draw(); CALL(glClear, m); depth_reapply(); }
static __attribute__((used)) void w_glClientActiveTexture(unsigned t) { ENTER("glClientActiveTexture", glClientActiveTexture); ARGS(t,0); g_cli_unit = (t - 0x84C0) & 3; CALL(glClientActiveTexture, t); }
static __attribute__((used)) void w_glColor4ub(unsigned r, unsigned g, unsigned b, unsigned a) {
    ENTER("glColor4ub", glColor4ub); g_col[0] = r; g_col[1] = g; g_col[2] = b; g_col[3] = a;
    /* the glow pass adds off-screen texture 2 (blurred bright parts) onto the final image (texture 4) with SRC_ALPHA,ONE: scale its alpha */
    if (g_bloom != 1.0f && g_bound_tex == 2 && fbt_get(g_cur_fb) == 4) { a = (unsigned)((float)a * g_bloom + 0.5f); if (a > 255) a = 255; static int once; if (!once) { once = 1; fprintf(stderr, "[gl] bloom pass scaled by %.2f\n", (double)g_bloom); fflush(stderr); } }
    { unsigned ft = fbt_get(g_cur_fb); static int bn; if ((ft == 2 || ft == 3 || ft == 4) && bn < 40) { bn++; fprintf(stderr, "[bloomcolor] fbtex=%u tex=%u rgba=%u %u %u %u\n", ft, g_bound_tex, r, g, b, a); fflush(stderr); } }
    CALL(glColor4ub, r, g, b, a);
}
static __attribute__((used)) void w_glColorMask(unsigned r, unsigned g, unsigned b, unsigned a) { ENTER("glColorMask", glColorMask); g_cm = (r&1) | ((g&1)<<1) | ((b&1)<<2) | ((a&1)<<3); ULOG("colorMask %u%u%u%u", r&1, g&1, b&1, a&1); CALL(glColorMask, r, g, b, a); }
static __attribute__((used)) void w_glCullFace(unsigned m) { ENTER("glCullFace", glCullFace); ULOG("cullFace 0x%x", m); g_cull_mode = m; if (g_cullopt == 2) m = (m == 0x404) ? 0x405 : (m == 0x405 ? 0x404 : m); CALL(glCullFace, m); }
static __attribute__((used)) void w_glDepthFunc(unsigned f) { ENTER("glDepthFunc", glDepthFunc); if (g_sfilter && f == g_dfunc && !g_dd_on) { g_pf_skips++; return; } g_dfunc = f; if (g_dd_on && g_dd_fp) fprintf(g_dd_fp, "s %d depthfunc 0x%x\n", g_dd_n, f); ULOG("depthFunc 0x%x", f); ARGS(f,0); CALL(glDepthFunc, f); }
static __attribute__((used)) void w_glDepthMask(unsigned m) { ENTER("glDepthMask", glDepthMask); if (g_sfilter && (m & 1) == g_dmask && !g_dd_on) { g_pf_skips++; return; } g_dmask = m & 1; if (g_dd_on && g_dd_fp) fprintf(g_dd_fp, "s %d depthmask %u\n", g_dd_n, m & 1); CALL(glDepthMask, m); }
static __attribute__((used)) void w_glFrontFace(unsigned m) { ENTER("glFrontFace", glFrontFace); ULOG("frontFace 0x%x", m); g_front = m; CALL(glFrontFace, m); }
static __attribute__((used)) void w_glShadeModel(unsigned m) { ENTER("glShadeModel", glShadeModel); ARGS(m,0); CALL(glShadeModel, m); }
static __attribute__((used)) void w_glStencilFunc(unsigned f, int r, unsigned m) { ENTER("glStencilFunc", glStencilFunc); ARGS(f,r); CALL(glStencilFunc, f, r, m); }
static __attribute__((used)) void w_glStencilMask(unsigned m) { ENTER("glStencilMask", glStencilMask); CALL(glStencilMask, m); }
static __attribute__((used)) void w_glStencilOp(unsigned a, unsigned b, unsigned c) { ENTER("glStencilOp", glStencilOp); ARGS(a,b); CALL(glStencilOp, a, b, c); }
static __attribute__((used)) void w_glFinish(void) { ENTER("glFinish", glFinish); }
static __attribute__((used)) unsigned w_glGetError(void) { return 0; }   /* the real error state is polled once per frame in gl_present */

static __attribute__((used)) void w_glEnable(unsigned cap) {
    ENTER("glEnable", glEnable); int sf_b = en_bit(cap), sf_same = g_sfilter && sf_b >= 0 && cap != 0xDE1 && ((g_enbits >> sf_b) & 1); en_track(cap, 1); if (cap == 0xDE1) g_u_en[g_act_unit] = 1; ULOG("enable 0x%x", cap); ARGS(cap,0);
    if (cap == 0x8840 /* MATRIX_PALETTE_OES */ || cap == 0x8861 /* POINT_SPRITE_OES */) { first("glEnable(palette/pointsprite) ignored", glEnable); return; }
    if (g_nofog && cap == 0x0B60) return;
    if (g_nolight && (cap == 0x0B50 || (cap >= 0x4000 && cap <= 0x4007))) return;
    if (g_nocolormat && cap == 0x0B57) return;
    if (cap == 0x0B44) { g_cull_en = 1; if (g_cullopt == 1) return; }   /* cull=1: never cull */
    if (sf_same) { g_pf_skips++; return; }
    CALL(glEnable, cap);
}
static __attribute__((used)) void w_glDisable(unsigned cap) {
    ENTER("glDisable", glDisable); int sf_b = en_bit(cap), sf_same = g_sfilter && sf_b >= 0 && cap != 0xDE1 && !((g_enbits >> sf_b) & 1); en_track(cap, 0); if (cap == 0xDE1) g_u_en[g_act_unit] = 0; ULOG("disable 0x%x", cap); ARGS(cap,0);
    if (cap == 0x8840 || cap == 0x8861) return;
    if (g_nofog && cap == 0x0B60) return;
    if (g_nolight && (cap == 0x0B50 || (cap >= 0x4000 && cap <= 0x4007))) return;
    if (g_normalize && cap == 0x0BA1) { CALL(glEnable, cap); return; }   /* keep GL_NORMALIZE on (scaled modelview matrices) */
    if (cap == 0x0B44) g_cull_en = 0;
    if (sf_same) { g_pf_skips++; return; }
    CALL(glDisable, cap);
}
static __attribute__((used)) void w_glEnableClientState(unsigned a) {
    ENTER("glEnableClientState", glEnableClientState); int sf_same = g_sfilter && a >= 0x8074 && a <= 0x8076 && ((g_clbits >> (a - 0x8074)) & 1); cl_track(a, 1); if (a == 0x8078) g_u_tc[g_cli_unit] = 1; ULOG("enableClient 0x%x", a); ARGS(a,0);
    if (a == 0x8844 /* MATRIX_INDEX_ARRAY_OES */ || a == 0x86AD /* WEIGHT_ARRAY_OES */) { first("glEnableClientState(palette arrays) ignored", glEnableClientState); return; }
    if (sf_same) { g_pf_skips++; return; }
    CALL(glEnableClientState, a);
}
static __attribute__((used)) void w_glDisableClientState(unsigned a) {
    ENTER("glDisableClientState", glDisableClientState); int sf_same = g_sfilter && a >= 0x8074 && a <= 0x8076 && !((g_clbits >> (a - 0x8074)) & 1); cl_track(a, 0); if (a == 0x8078) g_u_tc[g_cli_unit] = 0; ARGS(a,0);
    if (a == 0x8844 || a == 0x86AD) return;
    if (sf_same) { g_pf_skips++; return; }
    CALL(glDisableClientState, a);
}

/* ---------------- viewport / scissor (letterboxed while drawing to the window) ---------------- */
static __attribute__((used)) void w_glViewport(int x, int y, int w, int h) {
    ENTER("glViewport", glViewport); ARGS(w,h); g_gvport[0] = x; g_gvport[1] = y; g_gvport[2] = w; g_gvport[3] = h;
    { static int n; if (n < 8) { n++; fprintf(stderr, "[gl] glViewport(%d,%d,%d,%d) fb_default=%u\n", x, y, w, h, g_default_fb_bound); fflush(stderr); } }
    if (g_default_fb_bound) CALL(glViewport, (int)(g_xoff + x * g_scale), (int)(g_yoff + y * g_scale), (int)(w * g_scale), (int)(h * g_scale));
    else CALL(glViewport, x, y, w, h);
}
static __attribute__((used)) void w_glScissor(int x, int y, int w, int h) {
    ENTER("glScissor", glScissor); g_sc[0] = x; g_sc[1] = y; g_sc[2] = w; g_sc[3] = h;
    if (g_default_fb_bound) CALL(glScissor, (int)(g_xoff + x * g_scale), (int)(g_yoff + y * g_scale), (int)(w * g_scale), (int)(h * g_scale));
    else CALL(glScissor, x, y, w, h);
}

/* ---------------- fog / light / material / texenv ---------------- */
static float g_fog_raw_s = 610.5f, g_fog_raw_e = 1100.f; static int g_fog_shifted; static float g_fog_sh;
static void fog_resync(void) {   /* the pull-back state changed: re-send the game's fog distances with/without the shift */
    extern float g_pull_scale; extern int g_cam_ok;
    float sh = (g_follow_active && !g_first_person_gl() && g_cam_ok) ? g_campull * g_pull_scale : 0.f;   /* the pull-back actually applied */
    if (sh < 1.f) sh = 0.f;
    if (fabsf(sh - g_fog_sh) < 4.f && (sh != 0.f) == (g_fog_sh != 0.f)) return;
    g_fog_sh = sh; g_fog_shifted = sh != 0.f; g_fog_s = g_fog_raw_s + sh; g_fog_e = g_fog_raw_e + sh;
    CALL(glFogf, 0x0B63, g_fog_s); CALL(glFogf, 0x0B64, g_fog_e);
}
static __attribute__((used)) void SOFT w_glFogf(unsigned p, float v) { ENTER("glFogf", glFogf);
    if (p == 0x0B63) g_fog_raw_s = v; else if (p == 0x0B64) g_fog_raw_e = v;
    if ((p == 0x0B63 || p == 0x0B64) && g_fog_shifted) v += g_fog_sh;   /* keep fog distances as the game meant them after the camera pull-back */
    if (p == 0x0B63) g_fog_s = v; else if (p == 0x0B64) g_fog_e = v; else if (p == 0x0B62) g_fog_d = v; else if (p == 0x0B65) g_fog_m = (unsigned)v; if (g_dd_on && g_dd_fp) fprintf(g_dd_fp, "s %d fogf 0x%x %.3f\n", g_dd_n, p, (double)v); ULOG("fogf 0x%x %d", p, (int)(v*1000)); ARGS(p,0);
    if (p == 0x0B65 /* FOG_MODE */ && glFogi) { ULOG("fog mode -> glFogi 0x%x", (unsigned)v); CALL(glFogi, p, (int)v); } else CALL(glFogf, p, v); }
static __attribute__((used)) void w_glFogfv(unsigned p, const float *v) { ENTER("glFogfv", glFogfv); ARGS(p,0); if (p == 0x0B66) { memcpy(g_fogc, v, 16); if (g_dd_on && g_dd_fp) fprintf(g_dd_fp, "s %d fogcolor %.3f %.3f %.3f %.3f\n", g_dd_n, (double)v[0], (double)v[1], (double)v[2], (double)v[3]); } if (p == 0x0B66) ULOG("fogColor %d %d %d %d", (int)(v[0]*255), (int)(v[1]*255), (int)(v[2]*255), (int)(v[3]*255)); CALL(glFogfv, p, v); }
static __attribute__((used)) void w_glLightModelfv(unsigned p, const float *v) { ENTER("glLightModelfv", glLightModelfv); ARGS(p,0); VLOG("lightModel 0x%x %d %d %d %d", p, (int)(v[0]*1000), (int)(v[1]*1000), (int)(v[2]*1000), (int)(v[3]*1000)); if (p == 0x0B53) memcpy(g_gl_amb, v, 16); CALL(glLightModelfv, p, v); }
static __attribute__((used)) void SOFT w_glLightf(unsigned l, unsigned p, float v) {
    ENTER("glLightf", glLightf); VLOG("lightf 0x%x 0x%x = %d", l, p, (int)(v*1000)); if (l == 0x4000 && p >= 0x1207 && p <= 0x1209) g_l_att[p - 0x1207] = v;
    if (glLightf) { CALL(glLightf, l, p, v); return; }
    /* this vitaGL build has no glLightf: pass attenuation through glLightfv instead (spot exponent/cutoff stay at their defaults) */
    if (p >= 0x1207 && p <= 0x1209) { float a4[4] = { v, 0, 0, 0 }; CALL(glLightfv, l, p, a4); }
}
static void mv_apply(const float *m, const float *v, float *o) { for (int r = 0; r < 4; r++) o[r] = m[r] * v[0] + m[4 + r] * v[1] + m[8 + r] * v[2] + m[12 + r] * v[3]; }
static __attribute__((used)) void w_glLightfv(unsigned l, unsigned p, const float *v) { ENTER("glLightfv", glLightfv);
    if (l == 0x4000) { if (p == 0x1200) memcpy(g_l_amb, v, 16); else if (p == 0x1201) memcpy(g_l_dif, v, 16); else if (p == 0x1202) memcpy(g_l_spec, v, 16); else if (p == 0x1203) mv_apply(g_mv, v, g_l_pos); } VLOG("lightfv 0x%x 0x%x = %d %d %d %d", l, p, (int)(v[0]*1000), (int)(v[1]*1000), (int)(v[2]*1000), (int)(v[3]*1000)); if (p == 0x1204 /* SPOT_DIRECTION: unsupported, raised GL_INVALID_ENUM every frame */) return;
    if (g_nospec && p == 0x1202) { static const float z[4] = {0, 0, 0, 1}; CALL(glLightfv, l, p, z); return; }
    CALL(glLightfv, l, p, v); }
static __attribute__((used)) void SOFT w_glMaterialf(unsigned f, unsigned p, float v) { ENTER("glMaterialf", glMaterialf); if (p == 0x1601) g_shin = v; VLOG("materialf 0x%x 0x%x = %d", f, p, (int)(v*1000)); CALL(glMaterialf, f, p, v); }
static __attribute__((used)) void w_glMaterialfv(unsigned f, unsigned p, const float *v) { ENTER("glMaterialfv", glMaterialfv); if (p == 0x1600) memcpy(g_emis, v, 16); if (p == 0x1202) memcpy(g_mat_spec, v, 16); if (p == 0x1601) g_shin = v[0]; VLOG("materialfv 0x%x 0x%x = %d %d %d %d", f, p, (int)(v[0]*1000), (int)(v[1]*1000), (int)(v[2]*1000), (int)(v[3]*1000));
    if (g_nospec && p == 0x1202) { static const float z[4] = {0, 0, 0, 1}; CALL(glMaterialfv, f, p, z); return; }
    CALL(glMaterialfv, f, p, v); }
static __attribute__((used)) void SOFT w_glTexEnvf(unsigned t, unsigned p, float v) { ENTER("glTexEnvf", glTexEnvf); if (p == 0x2200) { g_tenv = (unsigned)v; g_u_env[g_act_unit] = (unsigned)v; } ULOG("texEnv 0x%x 0x%x %d", t, p, (int)v); ARGS(p,(int)v); CALL(glTexEnvf, t, p, v); }

/* ---------------- matrices ---------------- */
static __attribute__((used)) void w_glMatrixMode(unsigned m) { ENTER("glMatrixMode", glMatrixMode); g_matmode = m; ARGS(m,0); CALL(glMatrixMode, m); }
static __attribute__((used)) void w_glLoadIdentity(void) { ENTER("glLoadIdentity", glLoadIdentity); { float *cm = cur_mat(); if (cm) { memset(cm, 0, 64); cm[0] = cm[5] = cm[10] = cm[15] = 1; } } CALL(glLoadIdentity); }
/* projection tweaks: first-person (VIEW) gets a fixed horizontal FOV (fpfov); otherwise fov= widens every perspective.
 * The game may set its projection with LoadMatrixf or LoadIdentity+MultMatrixf, so both go through here. */
static const float *proj_adjust(const float *m, float *tmp) {
    extern int g_first_person;
    if (g_matmode == 0x1701 && g_follow_active && !g_first_person && g_campull > 0.f && m[11] < -0.99f && m[11] > -1.01f && m[15] > -0.01f && m[15] < 0.01f && m[10] < -1.0001f) {
        /* pull-back: move the far plane out by the same distance, or distant geometry is clipped (black wall, update177) */
        float n = m[14] / (m[10] - 1.f), f = m[14] / (m[10] + 1.f);
        /* update191: and move the near plane out too. Everything now sits ~165 further away, and depth precision falls with
         * distance squared, so coplanar layers (floor decals) z-fought - floors "fading in and out". Near *= ((250+pull)/250)^2
         * (max 3) gives back the precision the game had. The HUD is not affected: it is drawn
         * under an orthographic projection (update191's HUD scaling made it grow/shrink - removed in 192). */
        if (n > 0.f && f > n) { extern float g_pull_scale; extern int g_cam_ok;
            float pe = g_cam_ok ? g_campull * g_pull_scale : 0.f, k = (250.f + pe) / 250.f; k *= k; if (k > 3.f) k = 3.f; if (k < 1.f || g_nearboost == 0) k = 1.f; if (g_nearboost > 1 && pe > 0.f) k = (float)g_nearboost;   /* nearboost=N: fixed xN (test) */
            float f2 = f + g_campull * 1.5f, n2 = n * k; if (n2 > f2 * 0.5f) { n2 = n; k = 1.f; }
            g_near_k = k;
            memcpy(tmp, m, 64); tmp[10] = -(f2 + n2) / (f2 - n2); tmp[14] = -2.f * f2 * n2 / (f2 - n2); m = tmp;
            static unsigned c; if ((++c % 150) == 1) plog("[near] near %.2f -> %.2f far %.0f -> %.0f (pull %.0f)", n, n2, f, f2, pe); } }
    else if (g_matmode == 0x1701) g_near_k = 1.f;
    if (g_matmode != 0x1701 || !(m[11] < -0.99f && m[11] > -1.01f && m[15] > -0.01f && m[15] < 0.01f && m[0] > 0.01f)) return m;
    if (g_first_person && g_follow_active && g_fpfov > 10.f && g_fpfov < 170.f) {   /* follow-gated: menus/maps are 3D too (update180) */
        float aspect = m[5] / m[0], sx = 1.f / tanf(g_fpfov * 0.5f * 3.14159265f / 180.f), k = sx / m[0];
        memcpy(tmp, m, 64); tmp[0] = sx; tmp[5] = sx * aspect; tmp[8] *= k; tmp[9] *= k; return tmp; }
    if (g_fov > 0.5f && g_fov != 1.f) { memcpy(tmp, m, 64); tmp[0] /= g_fov; tmp[5] /= g_fov; tmp[8] /= g_fov; tmp[9] /= g_fov; return tmp; }
    return m;
}
/* third-person pull-back: only while the game's follow camera is running (camera hook ticked during the last frame) and
 * not in first person. Modelview loads under a perspective projection get an extra view-space translation (scene pushed
 * away and down = camera further back and higher). Menus/title run without the camera update, so they are untouched. */
/* HUD/2D: drawn under the same perspective, but with a modelview that has no rotation (axis-aligned quads). The world's
 * modelviews always carry the camera's pitch, so they never match. The pull-back must not move the HUD (update182:
 * the HUD slid with the camera pitch). */
/* update190: the HUD test (no rotation in the modelview) is only trustworthy while the game camera is tilted. When the
 * camera is level (its resting pose: start of story mode, after first person) and lined up behind the hero, the hero,
 * his weapons and anything facing his way also have no rotation in their modelview - they were skipped and floated
 * while the level moved back. With a level camera everything is moved (the lift is constant then, so the HUD does not slide). */
static int g_view_diag_gl(void) { extern int g_view_diag; extern float g_cam_sinp; return g_view_diag || (g_cam_sinp < 0.01f && g_cam_sinp > -0.01f); }
static int g_camreal_gl(void) { extern int g_vreal_ok; return g_vreal_ok; }
static int g_cam_ok_gl(void) { extern int g_cam_ok; return g_cam_ok; }
float gl_campull(void) { return g_campull; }
float gl_camraise(void) { return g_camraise; }
static int screen_aligned(const float *m) {
    float sc = fabsf(m[0]) + fabsf(m[5]) + fabsf(m[10]), off = fabsf(m[1]) + fabsf(m[2]) + fabsf(m[4]) + fabsf(m[6]) + fabsf(m[8]) + fabsf(m[9]);
    return off <= sc * 1e-4f;
}
static __attribute__((used)) void w_glLoadMatrixf(const float *m) { ENTER("glLoadMatrixf", glLoadMatrixf);
    float tmp[16];
    m = proj_adjust(m, tmp);
    if (g_verbose && g_matmode == 0x1700 && g_pj[11] < -0.99f && g_pj[11] > -1.01f && m[15] > 0.99f && m[15] < 1.01f) {   /* update189 diagnostics */
        extern int g_first_person; int dt = (g_enbits >> en_bit(0xB71)) & 1;
        if (!g_follow_active) g_pd[0]++; else if (g_first_person) g_pd[1]++; else if (!g_cam_ok_gl()) g_pd[2]++;
        else if (screen_aligned(m) && !g_view_diag_gl()) g_pd[3 + dt]++; else g_pd[5 + dt]++;
        if (screen_aligned(m)) { static unsigned k; if ((++k % 97) == 0) plog("[sa] m0 %.3f m5 %.3f m10 %.3f t %.1f %.1f %.1f dt %d dmask %d level %d", m[0], m[5], m[10], m[12], m[13], m[14], dt, g_dmask, g_view_diag_gl()); } }
    { extern int g_first_person;
      if (g_matmode == 0x1700 && g_follow_active && !g_first_person && (g_campull != 0.f || g_camraise != 0.f) && g_pj[11] < -0.99f && g_pj[11] > -1.01f && m[15] > 0.99f && m[15] < 1.01f && g_cam_ok_gl() && !g_camreal_gl() && !(screen_aligned(m) && !g_view_diag_gl())) {
        extern float g_pull_scale, g_cam_cosp, g_cam_sinp;
        float pull = g_campull * g_pull_scale, dh = g_camraise * g_cam_cosp + pull * g_cam_sinp, e = dh < 0.f ? -dh : 0.f;   /* e: world-up lift to stay above the game camera */
        memcpy(tmp, m, 64); tmp[14] -= pull + e * g_cam_sinp; tmp[13] -= g_camraise + e * g_cam_cosp; m = tmp; } }
    { float *cm = cur_mat(); if (cm) memcpy(cm, m, 64); } CALL(glLoadMatrixf, m); }
static __attribute__((used)) void w_glMultMatrixf(const float *m) { ENTER("glMultMatrixf", glMultMatrixf);
    float tmp2[16]; m = proj_adjust(m, tmp2);
    { float *cm = cur_mat(); if (cm) { float o[16]; for (int c = 0; c < 4; c++) for (int r = 0; r < 4; r++) o[c * 4 + r] = cm[r] * m[c * 4] + cm[4 + r] * m[c * 4 + 1] + cm[8 + r] * m[c * 4 + 2] + cm[12 + r] * m[c * 4 + 3]; memcpy(cm, o, 64); } } CALL(glMultMatrixf, m); }

/* ---------------- matrix palette skinning: not supported by vitaGL yet (meshes will draw unskinned) ---------------- */
static unsigned g_pal_calls, g_pal_maxidx, g_mip_calls, g_wp_calls;
static int g_pl_n;   /* separate small log budget for the palette calls */
#define PLOG(...) do { if (g_pl_n < 60) { g_pl_n++; fprintf(stderr, "[pal] " __VA_ARGS__); fputc(10, stderr); fflush(stderr); } } while (0)
static __attribute__((used)) void w_glCurrentPaletteMatrixOES(unsigned i) { ENTER("glCurrentPaletteMatrixOES (UNSUPPORTED)", NULL); if (i > g_pal_maxidx) { g_pal_maxidx = i; PLOG("palette index max now %u", i); } g_pal_calls++; }
static __attribute__((used)) void w_glLoadPaletteFromModelViewMatrixOES(void) { ENTER("glLoadPaletteFromModelViewMatrixOES (UNSUPPORTED)", NULL); g_pal_calls++; }
static __attribute__((used)) void w_glMatrixIndexPointerOES(int s, unsigned t, int st, const void *p) { ENTER("glMatrixIndexPointerOES (UNSUPPORTED)", NULL); if (g_mip_calls++ < 6) PLOG("matrixIndexPointer size=%d type=0x%x stride=%d ptr=%p arrayBuf=%u", s, t, st, p, g_arr_buf); }
static __attribute__((used)) void w_glWeightPointerOES(int s, unsigned t, int st, const void *p) { ENTER("glWeightPointerOES (UNSUPPORTED)", NULL); if (g_wp_calls++ < 6) PLOG("weightPointer size=%d type=0x%x stride=%d ptr=%p arrayBuf=%u", s, t, st, p, g_arr_buf); }

/* ---------------- vertex arrays / drawing ---------------- */
static __attribute__((used)) void w_glVertexPointer(int s, unsigned t, int st, const void *p) { ENTER("glVertexPointer", glVertexPointer); g_vp[0] = s; g_vp[1] = t; g_vp[2] = st; g_ap[0] = (ArrP){s, t, st, p, g_arr_buf}; ULOG("vertexPtr %d 0x%x stride=%d", s, t, st); ARGS(s,t); CALL(glVertexPointer, s, t, st, p); }
static __attribute__((used)) void w_glNormalPointer(unsigned t, int st, const void *p) { ENTER("glNormalPointer", glNormalPointer); g_np[0] = 3; g_np[1] = t; g_np[2] = st; g_ap[1] = (ArrP){3, t, st, p, g_arr_buf}; ULOG("normalPtr 0x%x stride=%d", t, st); CALL(glNormalPointer, t, st, p); }
static __attribute__((used)) void w_glColorPointer(int s, unsigned t, int st, const void *p) { ENTER("glColorPointer", glColorPointer); g_cp[0] = s; g_cp[1] = t; g_cp[2] = st; g_ap[2] = (ArrP){s, t, st, p, g_arr_buf}; ULOG("colorPtr %d 0x%x stride=%d", s, t, st); ARGS(s,t); CALL(glColorPointer, s, t, st, p); }
static __attribute__((used)) void w_glTexCoordPointer(int s, unsigned t, int st, const void *p) { ENTER("glTexCoordPointer", glTexCoordPointer); g_tp[0] = s; g_tp[1] = t; g_tp[2] = st; g_ap[3] = (ArrP){s, t, st, p, g_arr_buf}; ULOG("texCoordPtr %d 0x%x stride=%d", s, t, st); ARGS(s,t); CALL(glTexCoordPointer, s, t, st, p); }

/* Lit draws: compute the OpenGL fixed-function result on the CPU (single light, GL_COLOR_MATERIAL ambient+diffuse = vertex colour, alpha = vertex alpha),
 * then draw those colours with lighting off from client arrays. Returns 0 (caller draws normally) if anything needed is missing. */
static uint8_t *g_tc; static size_t g_tc_cap; static int g_cl_log;
static const uint8_t *arr_base(const ArrP *a, int first, int count, int elem) {
    int st = a->stride ? a->stride : elem; long end = (long)(first + count - 1) * st + elem;
    if (a->buf) { Shad *sh = shad_find(a->buf); if (!sh || (long)(uintptr_t)a->ptr < 0 || (long)(uintptr_t)a->ptr + end > sh->size) return NULL; return sh->d + (uintptr_t)a->ptr; }
    return (const uint8_t *)a->ptr;
}
static int cl_fail(int why, unsigned mode, int first, int count) {
    g_cl_why[why]++; static int n; if (g_cl_why[why] <= 2 && n++ < 14) { Shad *a0 = shad_find(g_ap[0].buf), *a2 = shad_find(g_ap[2].buf);
        fprintf(stderr, "[cpulight] fail %d: count=%d first=%d en=0x%x cl=0x%x V(%d,0x%x,%d) N(0x%x,%d) C(%d,0x%x,%d) bufs=%u/%u/%u/%u shadows v=%d c=%d total=%d/%uKB\n", why, count, first, g_enbits, g_clbits, g_ap[0].size, g_ap[0].type, g_ap[0].stride, g_ap[1].type, g_ap[1].stride, g_ap[2].size, g_ap[2].type, g_ap[2].stride, g_ap[0].buf, g_ap[1].buf, g_ap[2].buf, g_ap[3].buf, a0 ? a0->size : -1, a2 ? a2->size : -1, g_sh_n, g_sh_total >> 10); fflush(stderr); }
    return 0;
}
/* CPU lighting of vertices [i0, i1) of one draw (run on the game thread and, for big draws, half of it on the light worker thread) */
typedef struct { const uint8_t *vb, *nb, *cb; int vst, nst, cst, first, count, point; float Lx, Ly, Lz; } LJ;
static void light_range(const LJ *j, int i0, int i1, unsigned *cn) {
    const uint8_t *vb = j->vb, *nb = j->nb, *cb = j->cb; int vst = j->vst, nst = j->nst, cst = j->cst, first = j->first, count = j->count, point = j->point; float Lx = j->Lx, Ly = j->Ly, Lz = j->Lz; (void)count;
    for (int i = i0; i < i1; i++) {
        const float *n = (const float *)(nb + (size_t)i * nst); const uint8_t *c = cb + (size_t)i * cst;
        float ex = g_mv[0] * n[0] + g_mv[4] * n[1] + g_mv[8] * n[2], ey = g_mv[1] * n[0] + g_mv[5] * n[1] + g_mv[9] * n[2], ez = g_mv[2] * n[0] + g_mv[6] * n[1] + g_mv[10] * n[2];
        float l2 = ex * ex + ey * ey + ez * ez; if (l2 > 0.f && (l2 < 0.998f || l2 > 1.002f)) { float inv = 1.f / sqrtf(l2); ex *= inv; ey *= inv; ez *= inv; }   /* unit normals (the usual case) skip the sqrt */
        float lx = Lx, ly = Ly, lz = Lz, att = 1.f;
        if (point) { const float *v = (const float *)(vb + (size_t)i * vst); float px = g_mv[0] * v[0] + g_mv[4] * v[1] + g_mv[8] * v[2] + g_mv[12], py = g_mv[1] * v[0] + g_mv[5] * v[1] + g_mv[9] * v[2] + g_mv[13], pz = g_mv[2] * v[0] + g_mv[6] * v[1] + g_mv[10] * v[2] + g_mv[14];
            lx = Lx / g_l_pos[3] - px; ly = Ly / g_l_pos[3] - py; lz = Lz / g_l_pos[3] - pz; float d = sqrtf(lx * lx + ly * ly + lz * lz); if (d > 0.f) { lx /= d; ly /= d; lz /= d; }
            float den = g_l_att[0] + g_l_att[1] * d + g_l_att[2] * d * d; att = den > 0.f ? 1.f / den : 1.f; }
        float ndl = ex * lx + ey * ly + ez * lz; if (ndl < 0.f) ndl = 0.f;
        float spf = 0.f;                                                 /* GL specular, infinite viewer: h = normalize(L + (0,0,1)) */
        if (ndl > 0.f && !g_nospec && (g_mat_spec[0] > 0.f || g_mat_spec[1] > 0.f || g_mat_spec[2] > 0.f)) {
            float hx = lx, hy = ly, hz = lz + 1.f, hl = sqrtf(hx * hx + hy * hy + hz * hz); if (hl > 0.f) { hx /= hl; hy /= hl; hz /= hl; }
            float ndh = ex * hx + ey * hy + ez * hz; if (ndh < 0.f) ndh = 0.f;
            spf = g_shin > 0.f ? powf(ndh, g_shin) : 1.f; spf *= att; if (i == 0) cn[3]++; }
        uint8_t *o = g_tc + (size_t)(first + i) * 4;
        for (int k = 0; k < 3; k++) {
            float m = c[k] * (1.f / 255.f);
            float r = g_dbgmode == 6 ? m : g_emis[k] + g_gl_amb[k] * m + att * (g_l_amb[k] * m + g_l_dif[k] * m * ndl) + spf * g_l_spec[k] * g_mat_spec[k];
            if (k == 0) { cn[0]++; if (r >= 1.f && m < 0.99f) cn[1]++; if (r <= 0.02f) cn[2]++; }
            r = r < 0.f ? 0.f : r > 1.f ? 1.f : r; o[k] = (uint8_t)(r * 255.f + 0.5f);
        }
        o[3] = c[3];
        if (i0 == 0 && g_cl_log < 3 && i == 0) { g_cl_log++; fprintf(stderr, "[cpulight] draw v%d n=(%.2f %.2f %.2f) col=%u,%u,%u,%u L=(%.2f %.2f %.2f w%.0f) amb=%.2f dif=%.2f -> %u,%u,%u,%u mv=[%.2f %.2f %.2f / %.2f %.2f %.2f / %.2f %.2f %.2f]\n", count, (double)n[0], (double)n[1], (double)n[2], c[0], c[1], c[2], c[3], (double)Lx, (double)Ly, (double)Lz, (double)g_l_pos[3], (double)g_l_amb[0], (double)g_l_dif[0], o[0], o[1], o[2], o[3], (double)g_mv[0], (double)g_mv[1], (double)g_mv[2], (double)g_mv[4], (double)g_mv[5], (double)g_mv[6], (double)g_mv[8], (double)g_mv[9], (double)g_mv[10]); fflush(stderr); }
    }
}
/* light worker: pinned to core 0 (idle while the game thread maxes core 1) */
static pthread_mutex_t g_lw_mx = PTHREAD_MUTEX_INITIALIZER; static pthread_cond_t g_lw_go = PTHREAD_COND_INITIALIZER, g_lw_fin = PTHREAD_COND_INITIALIZER;
static LJ g_lw_job; static int g_lw_i0, g_lw_i1, g_lw_pending, g_lw_ok; static int g_lightmt = 1; static unsigned g_lw_cn[4];
extern int sceKernelChangeThreadCpuAffinityMask(int thid, int mask) __attribute__((weak));
static void *light_worker(void *arg) {
    (void)arg; if (sceKernelChangeThreadCpuAffinityMask) sceKernelChangeThreadCpuAffinityMask(0, 0x10000);
    pthread_mutex_lock(&g_lw_mx);
    for (;;) {
        while (!g_lw_pending) pthread_cond_wait(&g_lw_go, &g_lw_mx);
        pthread_mutex_unlock(&g_lw_mx);
        unsigned cn[4] = {0, 0, 0, 0}; light_range(&g_lw_job, g_lw_i0, g_lw_i1, cn);
        pthread_mutex_lock(&g_lw_mx);
        for (int k = 0; k < 4; k++) g_lw_cn[k] = cn[k];
        g_lw_pending = 0; pthread_cond_signal(&g_lw_fin);
    }
    return NULL;
}
static void light_all(const LJ *j) {
    unsigned cn[4] = {0, 0, 0, 0};
    g_lightmt = g_lightmt_cfg;
    if (!g_lw_ok && g_lightmt) { pthread_t th; pthread_attr_t at; pthread_attr_init(&at); pthread_attr_setstacksize(&at, 32 * 1024); g_lw_ok = pthread_create(&th, &at, light_worker, NULL) == 0 ? 1 : -1;
        fprintf(stderr, "[cpulight] worker thread %s (core 0)\n", g_lw_ok > 0 ? "started" : "FAILED"); fflush(stderr); }
    if (g_lw_ok > 0 && g_lightmt && j->count >= 384) {
        int mid = j->count / 2;
        pthread_mutex_lock(&g_lw_mx); g_lw_job = *j; g_lw_i0 = mid; g_lw_i1 = j->count; g_lw_pending = 1; pthread_cond_signal(&g_lw_go); pthread_mutex_unlock(&g_lw_mx);
        light_range(j, 0, mid, cn);
        pthread_mutex_lock(&g_lw_mx); while (g_lw_pending) pthread_cond_wait(&g_lw_fin, &g_lw_mx); for (int k = 0; k < 4; k++) cn[k] += g_lw_cn[k]; pthread_mutex_unlock(&g_lw_mx);
        g_lw_jobs++;
    } else light_range(j, 0, j->count, cn);
    g_cl_vtx += cn[0]; g_cl_sat += cn[1]; g_cl_dark += cn[2]; g_spec_vtx += cn[3];
}

/* ---- GPU lighting: the game's lit draws (one directional light, GL_COLOR_MATERIAL, no specular, linear fog) done in a Cg shader instead of on the CPU.
   Compiled at the first lit draw; any failure is logged and the CPU path (cpu_light_draw) is used instead. vita.cfg gpulight=0 disables. ---- */
static const char *g_gl_vs =
"attribute vec3 aPos;\n"
"attribute vec3 aNrm;\n"
"attribute vec4 aCol;\n"
"attribute vec2 aTex;\n"
"uniform mat4 uMvp;\n"
"uniform mat4 uMv;\n"
"uniform vec4 uL;\n"
"uniform vec4 uAmb;\n"
"uniform vec4 uDif;\n"
"uniform vec4 uEmis;\n"
"uniform vec4 uFog;\n"
"varying vec4 vCol;\n"
"varying vec2 vTex;\n"
"varying float vFog;\n"
"void main() {\n"
"  gl_Position = uMvp * vec4(aPos, 1.0);\n"
"  vec3 n = normalize((uMv * vec4(aNrm, 0.0)).xyz);\n"
"  float ndl = max(dot(n, uL.xyz), 0.0);\n"
"  vCol = vec4(clamp(uEmis.rgb + aCol.rgb * (uAmb.rgb + uDif.rgb * ndl), 0.0, 1.0), aCol.a);\n"
"  vTex = aTex;\n"
"  float d = abs((uMv * vec4(aPos, 1.0)).z);\n"
"  vFog = uFog.w > 0.5 ? clamp((uFog.y - d) * uFog.z, 0.0, 1.0) : 1.0;\n"
"}\n";
static const char *g_gl_fs =
"precision mediump float;\n"
"uniform sampler2D uTex;\n"
"uniform vec4 uFogCol;\n"
"uniform vec4 uOpt;\n"
"varying vec4 vCol;\n"
"varying vec2 vTex;\n"
"varying float vFog;\n"
"void main() {\n"
"  vec4 c = vCol;\n"
"  if (uOpt.x > 0.5) c = c * texture2D(uTex, vTex);\n"
"  if (c.a <= uOpt.y) discard;\n"
"  c.rgb = mix(uFogCol.rgb, c.rgb, vFog);\n"
"  gl_FragColor = c;\n"
"}\n";
static unsigned g_gl_prog; static int g_gl_state;   /* 0 = not tried, 1 = ready, -1 = failed */
static int g_gl_u_mvp, g_gl_u_mv, g_gl_u_l, g_gl_u_amb, g_gl_u_dif, g_gl_u_emis, g_gl_u_fog, g_gl_u_fogcol, g_gl_u_opt, g_gl_u_tex;
static unsigned gl_compile(unsigned type, const char *src) {
    unsigned sh = glCreateShader(type); glShaderSource(sh, 1, &src, NULL); glCompileShader(sh);
    int ok = 0; glGetShaderiv(sh, 0x8B81, &ok);
    if (!ok) { char log[1024] = ""; if (glGetShaderInfoLog) glGetShaderInfoLog(sh, sizeof log, NULL, log); fprintf(stderr, "[gpulight] %s shader compile FAILED: %s\n", type == 0x8B31 ? "vertex" : "fragment", log); fflush(stderr); return 0; }
    return sh;
}
static int gpu_light_init(void) {
    if (g_gl_state) return g_gl_state > 0;
    g_gl_state = -1;
    if (!glCreateShader || !glShaderSource || !glCompileShader || !glGetShaderiv || !glCreateProgram || !glAttachShader || !glBindAttribLocation || !glLinkProgram || !glGetProgramiv ||
        !glUseProgram || !glGetUniformLocation || !glUniformMatrix4fv || !glUniform4fv || !glUniform1i || !glVertexAttribPointer || !glEnableVertexAttribArray || !glDisableVertexAttribArray) {
        fprintf(stderr, "[gpulight] shader functions missing in vitaGL -> CPU lighting\n"); fflush(stderr); return 0; }
    { FILE *mk = fopen(GAME_DIR "gpulight.try", "r");
      if (mk) { fclose(mk); fprintf(stderr, "[gpulight] previous shader attempt crashed (marker ux0:data/baroque/gpulight.try present) -> CPU lighting. Delete the marker to retry.\n"); fflush(stderr); return 0; } }
    { FILE *mk = fopen(GAME_DIR "gpulight.try", "w"); if (mk) { fputs("shader build in progress\n", mk); fclose(mk); } }
    unsigned vs = gl_compile(0x8B31, g_gl_vs), fs = vs ? gl_compile(0x8B30, g_gl_fs) : 0;
    if (!vs || !fs) { remove(GAME_DIR "gpulight.try"); return 0; }
    unsigned p = glCreateProgram(); glAttachShader(p, vs); glAttachShader(p, fs);
    glBindAttribLocation(p, 0, "aPos"); glBindAttribLocation(p, 1, "aNrm"); glBindAttribLocation(p, 2, "aCol"); glBindAttribLocation(p, 3, "aTex");
    glLinkProgram(p); int ok = 0; glGetProgramiv(p, 0x8B82, &ok);
    if (!ok) { char log[1024] = ""; if (glGetProgramInfoLog) glGetProgramInfoLog(p, sizeof log, NULL, log); fprintf(stderr, "[gpulight] link FAILED: %s\n", log); fflush(stderr); remove(GAME_DIR "gpulight.try"); return 0; }
    g_gl_prog = p;
    g_gl_u_mvp = glGetUniformLocation(p, "uMvp"); g_gl_u_mv = glGetUniformLocation(p, "uMv"); g_gl_u_l = glGetUniformLocation(p, "uL"); g_gl_u_amb = glGetUniformLocation(p, "uAmb");
    g_gl_u_dif = glGetUniformLocation(p, "uDif"); g_gl_u_emis = glGetUniformLocation(p, "uEmis"); g_gl_u_fog = glGetUniformLocation(p, "uFog"); g_gl_u_fogcol = glGetUniformLocation(p, "uFogCol");
    g_gl_u_opt = glGetUniformLocation(p, "uOpt"); g_gl_u_tex = glGetUniformLocation(p, "uTex");
    fprintf(stderr, "[gpulight] shader ready (uniforms mvp=%d mv=%d L=%d amb=%d dif=%d emis=%d fog=%d fogcol=%d opt=%d tex=%d)\n", g_gl_u_mvp, g_gl_u_mv, g_gl_u_l, g_gl_u_amb, g_gl_u_dif, g_gl_u_emis, g_gl_u_fog, g_gl_u_fogcol, g_gl_u_opt, g_gl_u_tex); fflush(stderr);
    g_gl_state = 1; return 1;
}
static int gpu_light_draw(unsigned mode, int first, int count) {
    if (!g_gpulight || g_dbgmode == 6 || g_dd_on) return 0;
    if ((g_enbits & ((1u << 4) | (1u << 10) | (1u << 7))) != ((1u << 4) | (1u << 10) | (1u << 7))) return 0;
    if ((g_clbits & 0x7) != 0x7 || g_l_pos[3] != 0.f || g_mat_spec[0] > 0.f || g_mat_spec[1] > 0.f || g_mat_spec[2] > 0.f) return 0;   /* directional light, no specular */
    if (g_ap[0].type != 0x1406 || g_ap[0].size != 3 || g_ap[1].type != 0x1406 || g_ap[2].type != 0x1401 || g_ap[2].size != 4) return 0;
    int tex = (g_clbits & 0x10) && g_u_en[0] && g_ap[3].type == 0x1406 && g_ap[3].size == 2;
    int fog = ((g_enbits >> 5) & 1) && !g_nofog && g_charfog;
    if (fog && g_fog_m != 0x2601) return 0;                 /* only linear fog is implemented */
    if (!gpu_light_init()) return 0;
    float mvp[16];
    for (int c = 0; c < 4; c++) for (int r = 0; r < 4; r++) { float a = 0; for (int k = 0; k < 4; k++) a += g_pj[k * 4 + r] * g_mv[c * 4 + k]; mvp[c * 4 + r] = a; }
    float L[4] = { g_l_pos[0], g_l_pos[1], g_l_pos[2], 0.f }; float ll = sqrtf(L[0] * L[0] + L[1] * L[1] + L[2] * L[2]); if (ll > 0.f) { L[0] /= ll; L[1] /= ll; L[2] /= ll; }
    float amb[4] = { g_gl_amb[0] + g_l_amb[0], g_gl_amb[1] + g_l_amb[1], g_gl_amb[2] + g_l_amb[2], 1.f };
    float fe = g_fog_e - g_fog_s; float fogv[4] = { g_fog_s, g_fog_e, fe != 0.f ? 1.f / fe : 0.f, fog ? 1.f : 0.f };
    float opt[4] = { tex ? 1.f : 0.f, ((g_enbits >> 3) & 1) && g_afunc == 0x204 ? g_aref : -1.f, 0.f, 0.f };
    CALL(glUseProgram, g_gl_prog);
    CALL(glUniformMatrix4fv, g_gl_u_mvp, 1, 0, mvp); CALL(glUniformMatrix4fv, g_gl_u_mv, 1, 0, g_mv);
    CALL(glUniform4fv, g_gl_u_l, 1, L); CALL(glUniform4fv, g_gl_u_amb, 1, amb); CALL(glUniform4fv, g_gl_u_dif, 1, g_l_dif); CALL(glUniform4fv, g_gl_u_emis, 1, g_emis);
    CALL(glUniform4fv, g_gl_u_fog, 1, fogv); CALL(glUniform4fv, g_gl_u_fogcol, 1, g_fogc); CALL(glUniform4fv, g_gl_u_opt, 1, opt);
    if (g_gl_u_tex >= 0) CALL(glUniform1i, g_gl_u_tex, 0);
    static const unsigned char norm[4] = { 0, 0, 1, 0 };
    for (int a = 0; a < 4; a++) {
        if (a == 3 && !tex) { CALL(glDisableVertexAttribArray, 3); continue; }
        CALL(glBindBuffer, 0x8892, g_ap[a].buf); CALL(glVertexAttribPointer, (unsigned)a, a == 1 ? 3 : g_ap[a].size, g_ap[a].type, norm[a], g_ap[a].stride, g_ap[a].ptr); CALL(glEnableVertexAttribArray, (unsigned)a);
    }
    CALL(glDrawArrays, mode, first, count);
    for (int a = 0; a < 4; a++) CALL(glDisableVertexAttribArray, (unsigned)a);
    CALL(glUseProgram, 0);
    CALL(glBindBuffer, 0x8892, g_arr_buf);
    g_gl_gpu_draws++;
    { static int ok_once; if (!ok_once) { ok_once = 1; remove(GAME_DIR "gpulight.try"); fprintf(stderr, "[gpulight] first GPU-lit draw done\n"); fflush(stderr); } }
    return 1;
}
static int cpu_light_draw_inner(unsigned mode, int first, int count);
static int cpu_light_draw(unsigned mode, int first, int count) { unsigned long long t0 = prof_now(); int r = gpu_light_draw(mode, first, count) || cpu_light_draw_inner(mode, first, count); g_pf_light += prof_now() - t0; return r; }
static int cpu_light_draw_inner(unsigned mode, int first, int count) {
    if (!g_cpulight || count <= 0 || first < 0) return 0;
    if ((g_enbits & ((1u << 4) | (1u << 10) | (1u << 7))) != ((1u << 4) | (1u << 10) | (1u << 7))) return 0;   /* lighting + light0 + colour material */
    if ((g_clbits & 0x7) != 0x7) return cl_fail(1, mode, first, count);                                         /* vertex + normal + colour arrays */
    if (g_ap[0].type != 0x1406 || g_ap[0].size != 3 || g_ap[1].type != 0x1406 || g_ap[2].type != 0x1401 || g_ap[2].size != 4) return cl_fail(2, mode, first, count);
    const uint8_t *vb = arr_base(&g_ap[0], first, count, 12), *nb = arr_base(&g_ap[1], first, count, 12), *cb = arr_base(&g_ap[2], first, count, 4);
    int tex = (g_clbits & 0x10) != 0; const uint8_t *tb = NULL;
    if (tex) { int es = g_ap[3].size * (g_ap[3].type == 0x1406 ? 4 : g_ap[3].type == 0x1402 ? 2 : 1); tb = arr_base(&g_ap[3], first, count, es); if (!tb) return cl_fail(3, mode, first, count); }
    if (!vb || !nb || !cb) return cl_fail(4, mode, first, count);
    size_t need = (size_t)(first + count) * 4; if (need > g_tc_cap) { uint8_t *n = realloc(g_tc, need); if (!n) return 0; g_tc = n; g_tc_cap = need; }
    int vst = g_ap[0].stride ? g_ap[0].stride : 12, nst = g_ap[1].stride ? g_ap[1].stride : 12, cst = g_ap[2].stride ? g_ap[2].stride : 4;
    float Lx = g_l_pos[0], Ly = g_l_pos[1], Lz = g_l_pos[2]; int point = g_l_pos[3] != 0.f;

    if (!point) { float l = sqrtf(Lx * Lx + Ly * Ly + Lz * Lz); if (l > 0.f) { Lx /= l; Ly /= l; Lz /= l; } }
    { unsigned long long tl = prof_now(); LJ j = { vb, nb, cb, vst, nst, cst, first, count, point, Lx, Ly, Lz }; light_all(&j); g_pf_lloop += prof_now() - tl; }
    g_cl_ok++;
    /* draw from client arrays with lighting off */
    CALL(glBindBuffer, 0x8892, 0);
    CALL(glVertexPointer, 3, 0x1406, g_ap[0].stride, vb);
    CALL(glColorPointer, 4, 0x1401, 4, g_tc);
    if (tex) CALL(glTexCoordPointer, g_ap[3].size, g_ap[3].type, g_ap[3].stride, tb);
    CALL(glDisable, 0x0B50);
    CALL(glDrawArrays, mode, first, count);
    CALL(glEnable, 0x0B50);
    for (int a = 0; a < 4; a++) {                       /* restore the game's pointers under their own buffer bindings */
        if (a == 3 && !tex) continue;
        CALL(glBindBuffer, 0x8892, g_ap[a].buf);
        if (a == 0) CALL(glVertexPointer, g_ap[0].size, g_ap[0].type, g_ap[0].stride, g_ap[0].ptr);
        else if (a == 1) CALL(glNormalPointer, g_ap[1].type, g_ap[1].stride, g_ap[1].ptr);
        else if (a == 2) CALL(glColorPointer, g_ap[2].size, g_ap[2].type, g_ap[2].stride, g_ap[2].ptr);
        else CALL(glTexCoordPointer, g_ap[3].size, g_ap[3].type, g_ap[3].stride, g_ap[3].ptr);
    }
    CALL(glBindBuffer, 0x8892, g_arr_buf);
    return 1;
}

static void reupload_dyn(void) {
    if (!g_reup || !g_cpulight || !g_ap[0].buf || g_ap[0].type != 0x1406 || g_ap[0].size < 3) return;
    Shad *sh = shad_find(g_ap[0].buf);
    if (!sh || !sh->lastsrc || sh->nsub <= 0 || sh->lastsz < 12 || sh->lastoff < 0 || sh->lastoff + sh->lastsz > sh->size) return;
    const float *src = (const float *)sh->lastsrc, *up = (const float *)(sh->d + sh->lastoff);
    if (isfinite(up[0]) && isfinite(up[1]) && isfinite(up[2])) return;           /* what was uploaded is fine */
    if (!isfinite(src[0]) || !isfinite(src[1]) || !isfinite(src[2])) { g_reup_bad++; return; }   /* game memory is not valid either */
    memcpy(sh->d + sh->lastoff, sh->lastsrc, (size_t)sh->lastsz);
    unsigned prev = g_arr_buf; CALL(glBindBuffer, 0x8892, sh->id); CALL(glBufferSubData, 0x8892, sh->lastoff, sh->lastsz, sh->lastsrc); CALL(glBindBuffer, 0x8892, prev);
    g_reup_n++;
}
static int dbg_lit(void) { return (g_enbits & ((1u << 4) | (1u << 10))) == ((1u << 4) | (1u << 10)); }
static int dbg_trans(void) { return fbt_get(g_cur_fb) == 1 && !dbg_lit() && ((g_enbits >> 1) & 1) && !g_dmask; }   /* translucent, no depth write, scene framebuffer */
/* does this draw reference any vertex in front of the near plane (clip z < -w)? uses shadow copies + tracked matrices. -1 = unknown */
static int fx_crosses(int arrays, int first, int count, unsigned itype, const void *ip) {
    if (count <= 0 || g_ap[0].type != 0x1406 || g_ap[0].size < 3) return -1;
    int lo = first, hi = first + count - 1; const uint8_t *ib = NULL; int isz = 0;
    if (!arrays) {
        isz = itype == 0x1403 ? 2 : itype == 0x1401 ? 1 : 0; if (!isz) return -1;
        if (g_elem_buf) { Shad *es = shad_find(g_elem_buf); size_t off = (size_t)(uintptr_t)ip; if (!es || off + (size_t)count * isz > (size_t)es->size) return -1; ib = es->d + off; }
        else ib = (const uint8_t *)ip;
        lo = 1 << 30; hi = -1;
        for (int i = 0; i < count; i++) { int v = isz == 2 ? ((const uint16_t *)ib)[i] : ib[i]; if (v < lo) lo = v; if (v > hi) hi = v; }
        if (hi < 0) return -1;
    }
    const uint8_t *vb = arr_base(&g_ap[0], 0, hi + 1, 12); if (!vb) return -1;
    int st = g_ap[0].stride ? g_ap[0].stride : 12;
    float M[16];   /* M = P * MV (column-major) */
    for (int c = 0; c < 4; c++) for (int r = 0; r < 4; r++) { float a = 0; for (int k = 0; k < 4; k++) a += g_pj[k * 4 + r] * g_mv[c * 4 + k]; M[c * 4 + r] = a; }
    for (int i = lo; i <= hi; i++) { const float *v = (const float *)(vb + (size_t)i * st);
        float z = M[2] * v[0] + M[6] * v[1] + M[10] * v[2] + M[14], w = M[3] * v[0] + M[7] * v[1] + M[11] * v[2] + M[15];
        if (z < -w) return 1; }
    return 0;
}
static const void *g_skip_ip; static unsigned g_skip_it; static int g_skip_first, g_skip_cnt;
static int dbg_skip(int is_arrays) {
    if (!dbg_trans()) return 0;
    switch (g_dbgmode) {
    case 2: { int x = fx_crosses(is_arrays, g_skip_first, g_skip_cnt, g_skip_it, g_skip_ip); if (x > 0) g_xing_n++; else if (x == 0) g_xing_ok++; return x > 0; }
    case 7: return 1;
    }
    return 0;
}
/* CPU near-plane clipping for effect draws whose triangles pass behind the camera (the Vita path draws those as full-screen smears).
   Triangles are clipped against clip z + w >= eps in object space (the transform is linear, so attributes interpolate with the same t)
   and drawn as GL_TRIANGLES from an interleaved client array: pos f3 | colour ub4 | texcoord f2 (stride 24). Returns 1 if it drew. */
typedef struct { float p[3]; uint8_t c[4]; float t[2]; } CV;
static CV *g_cvo; static size_t g_cvo_cap; static float *g_cdist; static size_t g_cdist_cap; static int *g_cidx; static size_t g_cidx_cap;
static int fx_clip_draw(int arrays, unsigned mode, int first, int count, unsigned itype, const void *ip) {
    if ((mode != 4 && mode != 5 && mode != 6) || count < 3) return 0;
    if (g_ap[0].type != 0x1406 || g_ap[0].size < 3 || (g_clbits & 2)) { g_clip_fail++; return 0; }
    int hasc = (g_clbits & 4) != 0, hast = (g_clbits & 16) != 0;
    if (hasc && (g_ap[2].type != 0x1401 || g_ap[2].size != 4)) { g_clip_fail++; return 0; }
    if (hast && (g_ap[3].type != 0x1406 || g_ap[3].size != 2)) { g_clip_fail++; return 0; }
    if ((size_t)count > g_cidx_cap) { int *n = realloc(g_cidx, (size_t)count * sizeof(int)); if (!n) return 0; g_cidx = n; g_cidx_cap = (size_t)count; }
    int hi = -1;
    if (arrays) { for (int i = 0; i < count; i++) g_cidx[i] = first + i; hi = first + count - 1; }
    else {
        int isz = itype == 0x1403 ? 2 : itype == 0x1401 ? 1 : 0; if (!isz) { g_clip_fail++; return 0; }
        const uint8_t *ib;
        if (g_elem_buf) { Shad *es = shad_find(g_elem_buf); size_t off = (size_t)(uintptr_t)ip; if (!es || off + (size_t)count * isz > (size_t)es->size) { g_clip_fail++; return 0; } ib = es->d + off; }
        else ib = (const uint8_t *)ip;
        for (int i = 0; i < count; i++) { int v = isz == 2 ? ((const uint16_t *)ib)[i] : ib[i]; g_cidx[i] = v; if (v > hi) hi = v; }
    }
    if (hi < 0) return 0;
    const uint8_t *vb = arr_base(&g_ap[0], 0, hi + 1, 12), *cb = hasc ? arr_base(&g_ap[2], 0, hi + 1, 4) : NULL, *tb = hast ? arr_base(&g_ap[3], 0, hi + 1, 8) : NULL;
    if (!vb || (hasc && !cb) || (hast && !tb)) { g_clip_fail++; return 0; }
    int vst = g_ap[0].stride ? g_ap[0].stride : 12, cst = g_ap[2].stride ? g_ap[2].stride : 4, tst = g_ap[3].stride ? g_ap[3].stride : 8;
    float M[16];
    for (int c = 0; c < 4; c++) for (int r = 0; r < 4; r++) { float a = 0; for (int k = 0; k < 4; k++) a += g_pj[k * 4 + r] * g_mv[c * 4 + k]; M[c * 4 + r] = a; }
    if ((size_t)(hi + 1) > g_cdist_cap) { float *n = realloc(g_cdist, (size_t)(hi + 1) * sizeof(float)); if (!n) return 0; g_cdist = n; g_cdist_cap = (size_t)(hi + 1); }
    for (int i = 0; i <= hi; i++) { const float *v = (const float *)(vb + (size_t)i * vst);
        g_cdist[i] = (M[2] + M[3]) * v[0] + (M[6] + M[7]) * v[1] + (M[10] + M[11]) * v[2] + (M[14] + M[15]); }   /* clip z + w */
    int ntri = mode == 4 ? count / 3 : count - 2;
    size_t need = (size_t)ntri * 6; if (need > g_cvo_cap) { CV *n = realloc(g_cvo, need * sizeof(CV)); if (!n) return 0; g_cvo = n; g_cvo_cap = need; }
    const float eps = 1e-3f; int n = 0;
    #define LOADV(dst, idx) do { const float *_p = (const float *)(vb + (size_t)(idx) * vst); (dst).p[0] = _p[0]; (dst).p[1] = _p[1]; (dst).p[2] = _p[2]; \
        if (hasc) { memcpy((dst).c, cb + (size_t)(idx) * cst, 4); if (g_dbgmode == 4) (dst).c[3] = 0; } else { (dst).c[0] = (uint8_t)g_col[0]; (dst).c[1] = (uint8_t)g_col[1]; (dst).c[2] = (uint8_t)g_col[2]; (dst).c[3] = (uint8_t)g_col[3]; } \
        if (hast) { const float *_t = (const float *)(tb + (size_t)(idx) * tst); (dst).t[0] = _t[0]; (dst).t[1] = _t[1]; } else (dst).t[0] = (dst).t[1] = 0; } while (0)
    for (int t = 0; t < ntri; t++) {
        int ia, ib2, ic;
        if (mode == 4) { ia = g_cidx[t * 3]; ib2 = g_cidx[t * 3 + 1]; ic = g_cidx[t * 3 + 2]; }
        else if (mode == 5) { if (t & 1) { ia = g_cidx[t + 1]; ib2 = g_cidx[t]; ic = g_cidx[t + 2]; } else { ia = g_cidx[t]; ib2 = g_cidx[t + 1]; ic = g_cidx[t + 2]; } }
        else { ia = g_cidx[0]; ib2 = g_cidx[t + 1]; ic = g_cidx[t + 2]; }
        if (ia == ib2 || ib2 == ic || ia == ic) continue;                     /* degenerate strip joints */
        int id[3] = { ia, ib2, ic }; float d[3] = { g_cdist[ia] - eps, g_cdist[ib2] - eps, g_cdist[ic] - eps };
        int in = (d[0] >= 0) + (d[1] >= 0) + (d[2] >= 0);
        if (in == 0) continue;
        CV v[3]; for (int k = 0; k < 3; k++) LOADV(v[k], id[k]);
        if (in == 3) { g_cvo[n++] = v[0]; g_cvo[n++] = v[1]; g_cvo[n++] = v[2]; continue; }
        CV poly[4]; int pn = 0;                                                   /* Sutherland-Hodgman against one plane */
        for (int k = 0; k < 3; k++) { int k2 = (k + 1) % 3;
            if (d[k] >= 0) poly[pn++] = v[k];
            if ((d[k] >= 0) != (d[k2] >= 0)) { float s2 = d[k] / (d[k] - d[k2]); CV o;
                for (int j = 0; j < 3; j++) o.p[j] = v[k].p[j] + s2 * (v[k2].p[j] - v[k].p[j]);
                for (int j = 0; j < 4; j++) { float c = v[k].c[j] + s2 * ((float)v[k2].c[j] - (float)v[k].c[j]); o.c[j] = (uint8_t)(c < 0 ? 0 : c > 255 ? 255 : c + 0.5f); }
                for (int j = 0; j < 2; j++) o.t[j] = v[k].t[j] + s2 * (v[k2].t[j] - v[k].t[j]);
                poly[pn++] = o; } }
        for (int k = 1; k + 1 < pn; k++) { g_cvo[n++] = poly[0]; g_cvo[n++] = poly[k]; g_cvo[n++] = poly[k + 1]; }
    }
    #undef LOADV
    g_clip_draws++; g_clip_tin += ntri; g_clip_tout += n / 3;
    if (n) {
        CALL(glBindBuffer, 0x8892, 0);
        CALL(glVertexPointer, 3, 0x1406, sizeof(CV), &g_cvo[0].p);
        if (hasc) CALL(glColorPointer, 4, 0x1401, sizeof(CV), &g_cvo[0].c);
        if (hast) CALL(glTexCoordPointer, 2, 0x1406, sizeof(CV), &g_cvo[0].t);
        CALL(glDrawArrays, 4, 0, n);
        CALL(glBindBuffer, 0x8892, g_ap[0].buf); CALL(glVertexPointer, g_ap[0].size, g_ap[0].type, g_ap[0].stride, g_ap[0].ptr);
        if (hasc) { CALL(glBindBuffer, 0x8892, g_ap[2].buf); CALL(glColorPointer, g_ap[2].size, g_ap[2].type, g_ap[2].stride, g_ap[2].ptr); }
        if (hast) { CALL(glBindBuffer, 0x8892, g_ap[3].buf); CALL(glTexCoordPointer, g_ap[3].size, g_ap[3].type, g_ap[3].stride, g_ap[3].ptr); }
        CALL(glBindBuffer, 0x8892, g_arr_buf);
    }
    return 1;
}
static int fx_try_clip(int arrays, unsigned mode, int first, int count, unsigned itype, const void *ip) {
    return 0;
    return fx_clip_draw(arrays, mode, first, count, itype, ip);
}
/* vitaGL's fixed-function shader takes alpha from the material when GL_COLOR_MATERIAL is on, even with lighting off, so per-vertex alpha fades are lost.
   Real GL ignores colour material when lighting is off, so it is switched off around unlit draws (mode 1 = game as-is keeps it). */
static int g_cm_off;
static void fog_pre(void) {
    g_fogx = 0; if (!((g_enbits >> 5) & 1)) return; g_fog_draws_on++;
    if (((g_dbgmode == 0 && !g_charfog) && dbg_lit()) || g_dbgmode == 4) { CALL(glDisable, 0xB60); g_fogx = 1; }
    else if (g_dbgmode == 5) { CALL(glFogf, 0x0B63, g_fog_s * 3.f); CALL(glFogf, 0x0B64, g_fog_e * 3.f); g_fogx = 2; }
}
static void fog_post(void) { if (g_fogx == 1) CALL(glEnable, 0xB60); else if (g_fogx == 2) { CALL(glFogf, 0x0B63, g_fog_s); CALL(glFogf, 0x0B64, g_fog_e); } g_fogx = 0; }
static void cm_pre(void) {
    fog_pre();
    g_cm_off = 0;
    return;
    CALL(glDisable, 0xB57); g_cm_off = 1;
}
static void cm_post(void) { fog_post(); if (g_cm_off) { CALL(glEnable, 0xB57); g_cm_off = 0; } }
static int g_fx_forced, g_fx_col, g_fx_fog, g_fx_elem, g_fx_sky, g_fx_tex; static uint8_t *g_fxc; static size_t g_fxc_cap; static uint8_t *g_fxi; static size_t g_fxi_cap;
static float fx_mul(void) { return g_fxalpha; }
/* scale the alpha of an effect draw: colour array copied to a client array with alpha*mul, or the current colour if no array.
   For glDrawElements the index range is found from the indices (element buffer shadow or client pointer); *pi is replaced by client indices. */
static void fx_alpha(int arrays, int first, int count, unsigned itype, const void **pi) {
    float mul = fx_mul(); if (mul >= 0.999f || count <= 0) return;
    if (!(g_clbits & 4)) { CALL(glColor4ub, g_col[0], g_col[1], g_col[2], (unsigned)(g_col[3] * mul + 0.5f)); g_fx_col = 1; g_fxa_why[1]++; return; }
    if (g_ap[2].type != 0x1401 || g_ap[2].size != 4) { g_fxa_why[2]++; return; }
    int lo = first, hi = first + count - 1;
    if (!arrays) {
        int isz = itype == 0x1403 ? 2 : itype == 0x1401 ? 1 : 0; if (!isz) { g_fxa_why[3]++; return; }
        const uint8_t *ib;
        if (g_elem_buf) { Shad *es = shad_find(g_elem_buf); size_t off = (size_t)(uintptr_t)*pi; if (!es || off + (size_t)count * isz > (size_t)es->size) { g_fxa_why[4]++; return; } ib = es->d + off; }
        else ib = (const uint8_t *)*pi;
        lo = 1 << 30; hi = -1;
        for (int i = 0; i < count; i++) { int v = isz == 2 ? ((const uint16_t *)ib)[i] : ib[i]; if (v < lo) lo = v; if (v > hi) hi = v; }
        if (hi < 0) return;
        if (g_elem_buf) { size_t need = (size_t)count * isz; if (need > g_fxi_cap) { uint8_t *n = realloc(g_fxi, need); if (!n) return; g_fxi = n; g_fxi_cap = need; } memcpy(g_fxi, ib, need); }
        (void)lo; lo = 0;
    }
    const uint8_t *cb = arr_base(&g_ap[2], 0, hi + 1, 4); if (!cb) { g_fxa_why[5]++; return; }
    size_t need = (size_t)(hi + 1) * 4; if (need > g_fxc_cap) { uint8_t *n = realloc(g_fxc, need); if (!n) return; g_fxc = n; g_fxc_cap = need; }
    int cst = g_ap[2].stride ? g_ap[2].stride : 4;
    for (int i = lo; i <= hi; i++) { const uint8_t *c = cb + (size_t)i * cst; uint8_t *o = g_fxc + (size_t)i * 4; o[0] = c[0]; o[1] = c[1]; o[2] = c[2]; o[3] = (uint8_t)(c[3] * mul + 0.5f); }
    CALL(glBindBuffer, 0x8892, 0); CALL(glColorPointer, 4, 0x1401, 4, g_fxc); CALL(glBindBuffer, 0x8892, g_arr_buf); g_fx_col = 2; g_fxa_why[0]++;
    if (!arrays && g_elem_buf) { CALL(glBindBuffer, 0x8893, 0); *pi = g_fxi; g_fx_elem = 1; }
}
static void dbg_pre(int arrays, int first, int count, unsigned itype, const void **pi) {
    g_fx_forced = g_fx_col = g_fx_fog = g_fx_elem = g_fx_sky = 0;
    if (!dbg_trans()) return;
    if (g_fxcull && (!arrays || g_fxcull > 1)) { CALL(glEnable, 0xB44); CALL(glCullFace, 0x405); g_fx_forced = 1; }
    fx_alpha(arrays, first, count, itype, pi);
    /* sky-only haze: projection whose z output equals 0.99999*w, so every fragment lands at the far plane and (LEQUAL) only shows where nothing opaque was drawn */
    if (0) { float P[16]; memcpy(P, g_pj, sizeof P); for (int c = 0; c < 4; c++) P[c * 4 + 2] = P[c * 4 + 3] * 0.99999f;
        CALL(glMatrixMode, 0x1701); CALL(glLoadMatrixf, P); CALL(glMatrixMode, g_matmode); CALL(glDepthFunc, 0x203); g_fx_sky = 1; }
}
static void dbg_post(void) {
    if (g_fx_forced) { if (!g_cull_en) CALL(glDisable, 0xB44); CALL(glCullFace, g_cull_mode ? g_cull_mode : 0x405); }
    if (g_fx_col == 1) CALL(glColor4ub, g_col[0], g_col[1], g_col[2], g_col[3]);
    if (g_fx_col == 2) { CALL(glBindBuffer, 0x8892, g_ap[2].buf); CALL(glColorPointer, g_ap[2].size, g_ap[2].type, g_ap[2].stride, g_ap[2].ptr); CALL(glBindBuffer, 0x8892, g_arr_buf); }
    if (g_fx_elem) CALL(glBindBuffer, 0x8893, g_elem_buf);
    if (g_fx_fog == 1) CALL(glEnable, 0xB60);
    if (g_fx_tex) { CALL(glEnable, 0xDE1); g_fx_tex = 0; }
    if (g_fx_col == 3) { if (g_clbits & 4) CALL(glEnableClientState, 0x8076); CALL(glColor4ub, g_col[0], g_col[1], g_col[2], g_col[3]); }
    if (g_fx_sky == 2) CALL(glDepthFunc, g_dfunc);
    else if (g_fx_sky) { CALL(glMatrixMode, 0x1701); CALL(glLoadMatrixf, g_pj); CALL(glMatrixMode, g_matmode); CALL(glDepthFunc, g_dfunc); }
    g_fx_forced = g_fx_col = g_fx_fog = g_fx_elem = g_fx_sky = 0;
}

/* colour-path self test: draws full-screen quads on the window framebuffer with KNOWN vertex colour (255,0,0,a=64) and KNOWN current colour (0,255,0,a=200),
   blend SRC_ALPHA/ONE_MINUS_SRC_ALPHA over black, and reads the pixel back. Red ~64 = vitaGL used the vertex colour + vertex alpha; green ~200 = it used the current colour.
   Each case adds one of the states the game's effect draws use (colour material, texture, fog, VBO, element buffer). Results go to loader.txt as [ctest] lines. */
static void color_selftest(const char *why) {
    static unsigned tex, vbo, ibo; static uint8_t px[32 * 32 * 4];
    if (!glReadPixels || !glGenTextures || !glGenBuffers) return;
    typedef struct { float p[3]; uint8_t c[4]; float t[2]; } TV;
    static TV q[4] = { {{-1,-1,0},{255,0,0,64},{0,0}}, {{1,-1,0},{255,0,0,64},{1,0}}, {{-1,1,0},{255,0,0,64},{0,1}}, {{1,1,0},{255,0,0,64},{1,1}} };
    static const uint16_t idx[4] = { 0, 1, 2, 3 };
    if (!tex) { static const uint32_t white[4] = { 0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu };
        CALL(glGenTextures, 1, &tex); CALL(glBindTexture, 0xDE1, tex); CALL(glTexImage2D, 0xDE1, 0, 0x1908, 2, 2, 0, 0x1908, 0x1401, white);
        CALL(glTexParameteri, 0xDE1, 0x2801, 0x2600); CALL(glTexParameteri, 0xDE1, 0x2800, 0x2600);
        CALL(glGenBuffers, 1, &vbo); CALL(glBindBuffer, 0x8892, vbo); CALL(glBufferData, 0x8892, (int)sizeof q, q, 0x88E4);
        CALL(glGenBuffers, 1, &ibo); CALL(glBindBuffer, 0x8893, ibo); CALL(glBufferData, 0x8893, (int)sizeof idx, idx, 0x88E4); }
    static const struct { const char *n; int vbo, elem, cm, tx, fog, carr, at; } cs[] = {
        { "client arrays, colour array (plain)",               0, 0, 0, 0, 0, 1 },
        { "client + colour material",                           0, 0, 1, 0, 0, 1 },
        { "client + texture",                                   0, 0, 0, 1, 0, 1 },
        { "client + fog",                                       0, 0, 0, 0, 1, 1 },
        { "client + colmat + texture + fog",                    0, 0, 1, 1, 1, 1 },
        { "VBO arrays (plain)",                                 1, 0, 0, 0, 0, 1 },
        { "VBO + element buffer (plain)",                       1, 1, 0, 0, 0, 1 },
        { "VBO + elements + colmat + texture + fog (= game effect)", 1, 1, 1, 1, 1, 1 },
        { "game effect but colour material OFF",                1, 1, 0, 1, 1, 1 },
        { "game effect, colour array DISABLED (current colour only)", 1, 1, 1, 1, 1, 0 },
        { "client + ALPHA TEST (func GREATER ref 0.016)",          0, 0, 0, 0, 0, 1, 1 },
        { "VBO+elements+colmat+tex+fog + ALPHA TEST (= game WORLD)", 1, 1, 1, 1, 1, 1, 1 },
        { "game WORLD, alpha test ref 0.7 (a=64 should be DISCARDED)", 1, 1, 1, 1, 1, 1, 2 },
        { "alpha test ref 0.7, NO blend (a=64 should be DISCARDED)",   0, 0, 0, 0, 0, 1, 3 },
    };
    CALL(glBindFramebuffer, 0x8D40, win_fb());
    CALL(glViewport, 0, 0, SCREEN_W, SCREEN_H);
    CALL(glDisable, 0xC11); CALL(glDisable, 0xB44); CALL(glDisable, 0xB71); CALL(glDisable, 0xBC0); CALL(glDisable, 0xB50); CALL(glDisable, 0xB90);
    CALL(glEnable, 0xBE2); CALL(glBlendFunc, 0x302, 0x303); CALL(glColorMask, 1, 1, 1, 1);
    CALL(glMatrixMode, 0x1701); CALL(glPushMatrix); CALL(glLoadIdentity); CALL(glMatrixMode, 0x1700); CALL(glPushMatrix); CALL(glLoadIdentity);
    CALL(glBindTexture, 0xDE1, tex); CALL(glTexEnvf, 0x2300, 0x2200, (float)0x2100);
    { static const float blk[4] = { 0, 0, 0, 1 }; CALL(glFogfv, 0x0B66, blk); }
    CALL(glDisableClientState, 0x8075);
    fprintf(stderr, "[ctest] ---- colour path self test (%s): vertex colour = 255,0,0 a64, current colour = 0,255,0 a200 ----\n", why);
    for (unsigned k = 0; k < sizeof cs / sizeof cs[0]; k++) {
        CALL(glClearColor, 0.f, 0.f, 0.f, 1.f); CALL(glClear, 0x4000);
        CALL(glColor4ub, 0, 255, 0, 200);
        if (cs[k].cm) CALL(glEnable, 0xB57); else CALL(glDisable, 0xB57);
        if (cs[k].tx) { CALL(glEnable, 0xDE1); CALL(glEnableClientState, 0x8078); } else { CALL(glDisable, 0xDE1); CALL(glDisableClientState, 0x8078); }
        if (cs[k].fog) CALL(glEnable, 0xB60); else CALL(glDisable, 0xB60);
        if (cs[k].at) { CALL(glEnable, 0xBC0); CALL(glAlphaFunc, 0x204, cs[k].at >= 2 ? 0.7f : 0.016f); } else CALL(glDisable, 0xBC0);
        if (cs[k].at == 3) CALL(glDisable, 0xBE2); else CALL(glEnable, 0xBE2);
        CALL(glEnableClientState, 0x8074);
        if (cs[k].carr) CALL(glEnableClientState, 0x8076); else CALL(glDisableClientState, 0x8076);
        const uint8_t *base = cs[k].vbo ? (const uint8_t *)0 : (const uint8_t *)q;
        CALL(glBindBuffer, 0x8892, cs[k].vbo ? vbo : 0);
        CALL(glVertexPointer, 3, 0x1406, (int)sizeof(TV), base);
        CALL(glColorPointer, 4, 0x1401, (int)sizeof(TV), base + 12);
        CALL(glTexCoordPointer, 2, 0x1406, (int)sizeof(TV), base + 16);
        if (cs[k].elem) { CALL(glBindBuffer, 0x8893, ibo); CALL(glDrawElements, 5, 4, 0x1403, (const void *)0); }
        else { CALL(glBindBuffer, 0x8893, 0); CALL(glDrawArrays, 5, 0, 4); }
        glReadPixels(SCREEN_W / 2 - 16, SCREEN_H / 2 - 16, 32, 32, 0x1908, 0x1401, px);
        const uint8_t *c = px + (16 * 32 + 16) * 4;
        fprintf(stderr, "[ctest] %-58s -> rgba %3u %3u %3u %3u  (%s)\n", cs[k].n, c[0], c[1], c[2], c[3],
                c[0] > 40 && c[0] < 90 && c[1] < 20 ? "VERTEX colour+alpha used (correct)" : c[1] > 150 && c[0] < 20 ? "CURRENT colour used" : c[0] > 200 ? "vertex rgb, alpha IGNORED (opaque)" : "other");
    }
    /* blend equation test: clear to grey 200, draw colour 100 (a=255) with blend SRC_ALPHA,ONE. ADD -> 255, REVERSE_SUBTRACT (dst-src) -> 100, SUBTRACT (src-dst) -> 0 */
    CALL(glDisable, 0xBC0); CALL(glDisable, 0xB57); CALL(glDisable, 0xDE1); CALL(glDisable, 0xB60); CALL(glDisableClientState, 0x8076); CALL(glDisableClientState, 0x8078);
    CALL(glBindBuffer, 0x8892, 0); CALL(glBindBuffer, 0x8893, 0); CALL(glVertexPointer, 3, 0x1406, (int)sizeof(TV), (const uint8_t *)q);
    CALL(glEnable, 0xBE2); CALL(glBlendFunc, 0x302, 1);
    static const unsigned eqs[3] = { 0x8006, 0x800B, 0x800A }; static const char *eqn[3] = { "FUNC_ADD (expect 255)", "FUNC_REVERSE_SUBTRACT (expect 100)", "FUNC_SUBTRACT (expect 0)" };
    for (int e = 0; e < 3; e++) {
        CALL(glClearColor, 200.f / 255.f, 200.f / 255.f, 200.f / 255.f, 1.f); CALL(glClear, 0x4000);
        CALL(glBlendEquation, eqs[e]); CALL(glColor4ub, 100, 100, 100, 255); CALL(glDrawArrays, 5, 0, 4);
        glReadPixels(SCREEN_W / 2 - 16, SCREEN_H / 2 - 16, 32, 32, 0x1908, 0x1401, px); const uint8_t *c = px + (16 * 32 + 16) * 4;
        fprintf(stderr, "[ctest] blend equation %-36s -> %u %u %u\n", eqn[e], c[0], c[1], c[2]);
    }
    CALL(glBlendEquation, g_beq);
    fflush(stderr);
    /* restore what the game believes is set */
    CALL(glMatrixMode, 0x1701); CALL(glPopMatrix); CALL(glMatrixMode, 0x1700); CALL(glPopMatrix); CALL(glMatrixMode, g_matmode);
    static const unsigned caps[11] = { 0xB44, 0xBE2, 0xB71, 0xBC0, 0xB50, 0xB60, 0xDE1, 0xB57, 0xB90, 0x4000, 0xC11 }; static const int bits[11] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 11 };
    for (int i = 0; i < 11; i++) { if ((g_enbits >> bits[i]) & 1) CALL(glEnable, caps[i]); else CALL(glDisable, caps[i]); }
    CALL(glAlphaFunc, g_afunc, g_aref); CALL(glBlendFunc, g_bs, g_bd); CALL(glColorMask, g_cm & 1, (g_cm >> 1) & 1, (g_cm >> 2) & 1, (g_cm >> 3) & 1);
    CALL(glFogfv, 0x0B66, g_fogc); CALL(glTexEnvf, 0x2300, 0x2200, (float)g_tenv); CALL(glBindTexture, 0xDE1, g_bound_tex);
    CALL(glClearColor, g_clr[0], g_clr[1], g_clr[2], g_clr[3]); CALL(glColor4ub, g_col[0], g_col[1], g_col[2], g_col[3]);
    static const unsigned csx[4] = { 0x8074, 0x8075, 0x8076, 0x8078 }; static const int cb[4] = { 0, 1, 2, 4 };
    for (int i = 0; i < 4; i++) { if ((g_clbits >> cb[i]) & 1) CALL(glEnableClientState, csx[i]); else CALL(glDisableClientState, csx[i]); }
    if (g_clbits & 1) { CALL(glBindBuffer, 0x8892, g_ap[0].buf); CALL(glVertexPointer, g_ap[0].size, g_ap[0].type, g_ap[0].stride, g_ap[0].ptr); }
    if (g_clbits & 2) { CALL(glBindBuffer, 0x8892, g_ap[1].buf); CALL(glNormalPointer, g_ap[1].type, g_ap[1].stride, g_ap[1].ptr); }
    if (g_clbits & 4) { CALL(glBindBuffer, 0x8892, g_ap[2].buf); CALL(glColorPointer, g_ap[2].size, g_ap[2].type, g_ap[2].stride, g_ap[2].ptr); }
    if (g_clbits & 16) { CALL(glBindBuffer, 0x8892, g_ap[3].buf); CALL(glTexCoordPointer, g_ap[3].size, g_ap[3].type, g_ap[3].stride, g_ap[3].ptr); }
    CALL(glBindBuffer, 0x8892, g_arr_buf); CALL(glBindBuffer, 0x8893, g_elem_buf);
    if (!g_default_fb_bound && g_cur_fb) CALL(glBindFramebuffer, 0x8D40, g_cur_fb);
}
/* on-screen mode indicator: N small white squares in the top-left corner for SELECT mode N (drawn with scissored clears, state restored) */
extern int pad_pointer(float *x, float *y);
static void draw_pointer(void) {   /* pointer mode (SELECT): a crosshair drawn with scissored clears (touches no other GL state) */
    float px, py; if (!pad_pointer(&px, &py)) return;
    px /= touch_sx();
    int sx = (int)(px * g_content_scale * g_scale + g_xoff), sy = SCREEN_H - (int)(py * g_content_scale * g_scale + g_yoff);
    CALL(glBindFramebuffer, 0x8D40, win_fb());
    CALL(glEnable, 0xC11); CALL(glColorMask, 1, 1, 1, 1);
    static const int R[6][4] = { {-13, -2, 26, 5}, {-2, -13, 5, 26}, {-11, -1, 22, 3}, {-1, -11, 3, 22}, {-3, -3, 7, 7}, {-2, -2, 5, 5} };
    for (int i = 0; i < 6; i++) {
        float c = (i == 4) ? 0.f : (i & 1) == 0 && i < 2 ? 0.f : 1.f;
        if (i < 2) CALL(glClearColor, 0.f, 0.f, 0.f, 1.f); else if (i == 4) CALL(glClearColor, 0.f, 0.f, 0.f, 1.f); else CALL(glClearColor, 1.f, 0.85f, 0.2f, 1.f);
        (void)c;
        int x = sx + R[i][0], y = sy + R[i][1], w = R[i][2], h = R[i][3]; if (x < 0) x = 0; if (y < 0) y = 0;
        CALL(glScissor, x, y, w, h); CALL(glClear, 0x4000);
    }
    CALL(glClearColor, g_clr[0], g_clr[1], g_clr[2], g_clr[3]);
    CALL(glColorMask, g_cm & 1, (g_cm >> 1) & 1, (g_cm >> 2) & 1, (g_cm >> 3) & 1);
    if (!((g_enbits >> 11) & 1)) CALL(glDisable, 0xC11);
    if (g_cur_fb && !g_default_fb_bound) CALL(glBindFramebuffer, 0x8D40, g_cur_fb);
    if (g_default_fb_bound) CALL(glScissor, (int)(g_xoff + g_sc[0] * g_scale), (int)(g_yoff + g_sc[1] * g_scale), (int)(g_sc[2] * g_scale), (int)(g_sc[3] * g_scale));
    else CALL(glScissor, g_sc[0], g_sc[1], g_sc[2], g_sc[3]);
}
static void draw_mode_indicator(void) {
    if (!g_dbgmode) return;
    CALL(glBindFramebuffer, 0x8D40, win_fb());
    CALL(glEnable, 0xC11); CALL(glColorMask, 1, 1, 1, 1); CALL(glClearColor, 1.f, 1.f, 1.f, 1.f);
    for (int i = 0; i < g_dbgmode; i++) { CALL(glScissor, 8 + i * 22, 544 - 24, 16, 16); CALL(glClear, 0x4000); }
    CALL(glClearColor, g_clr[0], g_clr[1], g_clr[2], g_clr[3]);
    CALL(glColorMask, g_cm & 1, (g_cm >> 1) & 1, (g_cm >> 2) & 1, (g_cm >> 3) & 1);
    if (!((g_enbits >> 11) & 1)) CALL(glDisable, 0xC11);
    if (g_cur_fb && !g_default_fb_bound) CALL(glBindFramebuffer, 0x8D40, g_cur_fb);
    if (g_default_fb_bound) CALL(glScissor, (int)(g_xoff + g_sc[0] * g_scale), (int)(g_yoff + g_sc[1] * g_scale), (int)(g_sc[2] * g_scale), (int)(g_sc[3] * g_scale));
    else CALL(glScissor, g_sc[0], g_sc[1], g_sc[2], g_sc[3]);
}

/* (state declared near g_draws) one-frame "frame stepping": after each draw into the scene framebuffer (late in the frame) read it back, halve it and append to snap.bin */
static void snap_after(void) {
    if (!g_dd_on || !g_snap_fp || !glReadPixels) return;
    int idx = g_dd_n - 1; if (idx < g_snap_from || fbt_get(g_cur_fb) != 1) return;
    static uint8_t rgba[480 * 320 * 4], out[240 * 160 * 3];
    glReadPixels(0, 0, 480, 320, 0x1908, 0x1401, rgba);
    for (int y = 0; y < 160; y++) for (int x = 0; x < 240; x++) for (int k = 0; k < 3; k++) {
        const uint8_t *q = rgba + ((size_t)(y * 2) * 480 + (size_t)(x * 2)) * 4 + k;
        out[((size_t)y * 240 + (size_t)x) * 3 + k] = (uint8_t)((q[0] + q[4] + q[480 * 4] + q[480 * 4 + 4]) >> 2);
    }
    fwrite(&idx, 4, 1, g_snap_fp); fwrite(out, 1, sizeof out, g_snap_fp);
}
/* mid-scene depth self-test (runs up to 3 times, after the game has been running a while, right after an effect draw into the scene framebuffer; all GL state it touches is restored from the shim's tracking).
   Draws a near red quad then a far green quad over it with the same state classes the game uses for effects. Red in the middle = the far quad was rejected by the depth test. */
static void depth_selftest2(void) {
    static unsigned last; static int runs;
    if (1 || runs >= 3 || g_frames < 900 || (runs && g_frames - last < 2000)) return;
    if (!dbg_trans()) return;
    runs++; last = g_frames;
    static const float nearq[12] = { -0.5f,-0.5f,-0.5f,  0.5f,-0.5f,-0.5f,  -0.5f,0.5f,-0.5f,  0.5f,0.5f,-0.5f };
    static const float farq[12]  = { -0.3f,-0.3f, 0.5f,  0.7f,-0.3f, 0.5f,  -0.3f,0.7f, 0.5f,  0.7f,0.7f, 0.5f };
    static const char *names[5] = { "far only", "near+far both write depth", "near, far no depth write", "near, far no depth write + blend a=255", "near, far no depth write + blend a=128" };
    unsigned char px[5][12]; memset(px, 0, sizeof px); static uint8_t full[480 * 320 * 4];
    CALL(glViewport, 0, 0, 480, 320);
    CALL(glDisable, 0xC11); CALL(glDisable, 0xB44); CALL(glDisable, 0xBE2); CALL(glDisable, 0xBC0); CALL(glDisable, 0xB50); CALL(glDisable, 0xB60); CALL(glDisable, 0xDE1);
    CALL(glEnable, 0xB71); CALL(glDepthFunc, 0x203); CALL(glColorMask, 1, 1, 1, 1);
    CALL(glMatrixMode, 0x1701); CALL(glPushMatrix); CALL(glLoadIdentity); CALL(glMatrixMode, 0x1700); CALL(glPushMatrix); CALL(glLoadIdentity);
    CALL(glBindBuffer, 0x8892, 0);
    CALL(glEnableClientState, 0x8074); CALL(glDisableClientState, 0x8075); CALL(glDisableClientState, 0x8076); CALL(glDisableClientState, 0x8078);
    for (int t = 0; t < 5; t++) {
        CALL(glDepthMask, 1); CALL(glClearColor, 0.f, 0.f, 0.f, 1.f); CALL(glClearDepthf, 1.f); CALL(glClear, 0x4100);
        if (t > 0) { CALL(glVertexPointer, 3, 0x1406, 0, nearq); CALL(glColor4ub, 255, 0, 0, 255); CALL(glDrawArrays, 5, 0, 4); }
        CALL(glDepthMask, t >= 2 ? 0 : 1);
        if (t >= 3) { CALL(glEnable, 0xBE2); CALL(glBlendFunc, 0x302, 0x303); }
        CALL(glVertexPointer, 3, 0x1406, 0, farq); CALL(glColor4ub, 0, 255, 0, t == 4 ? 128 : 255); CALL(glDrawArrays, 5, 0, 4);
        if (t >= 3) CALL(glDisable, 0xBE2);
        if (glReadPixels) { glReadPixels(0, 0, 480, 320, 0x1908, 0x1401, full);
            static const int sx[3] = { 240, 144, 384 }, sy[3] = { 160, 96, 256 };
            for (int k = 0; k < 3; k++) { const uint8_t *q = full + ((size_t)sy[k] * 480 + (size_t)sx[k]) * 4; px[t][k * 4] = q[0]; px[t][k * 4 + 1] = q[1]; px[t][k * 4 + 2] = q[2]; } }
    }
    CALL(glClear, 0x4100);
    /* restore everything the game believes is set */
    CALL(glMatrixMode, 0x1701); CALL(glPopMatrix); CALL(glMatrixMode, 0x1700); CALL(glPopMatrix); CALL(glMatrixMode, g_matmode);
    static const unsigned caps[10] = { 0xB44, 0xBE2, 0xB71, 0xBC0, 0xB50, 0xB60, 0xDE1, 0xB57, 0x4000, 0xC11 }; static const int bits[10] = { 0, 1, 2, 3, 4, 5, 6, 7, 10, 11 };
    for (int i = 0; i < 10; i++) { if ((g_enbits >> bits[i]) & 1) CALL(glEnable, caps[i]); else CALL(glDisable, caps[i]); }
    CALL(glDepthMask, g_dmask); CALL(glDepthFunc, g_dfunc); CALL(glBlendFunc, g_bs, g_bd); CALL(glColorMask, g_cm & 1, (g_cm >> 1) & 1, (g_cm >> 2) & 1, (g_cm >> 3) & 1);
    static const unsigned cs[4] = { 0x8074, 0x8075, 0x8076, 0x8078 }; static const int cb[4] = { 0, 1, 2, 4 };
    for (int i = 0; i < 4; i++) { if ((g_clbits >> cb[i]) & 1) CALL(glEnableClientState, cs[i]); else CALL(glDisableClientState, cs[i]); }
    if (g_clbits & 1) { CALL(glBindBuffer, 0x8892, g_ap[0].buf); CALL(glVertexPointer, g_ap[0].size, g_ap[0].type, g_ap[0].stride, g_ap[0].ptr); }
    if (g_clbits & 2) { CALL(glBindBuffer, 0x8892, g_ap[1].buf); CALL(glNormalPointer, g_ap[1].type, g_ap[1].stride, g_ap[1].ptr); }
    if (g_clbits & 4) { CALL(glBindBuffer, 0x8892, g_ap[2].buf); CALL(glColorPointer, g_ap[2].size, g_ap[2].type, g_ap[2].stride, g_ap[2].ptr); }
    if (g_clbits & 16) { CALL(glBindBuffer, 0x8892, g_ap[3].buf); CALL(glTexCoordPointer, g_ap[3].size, g_ap[3].type, g_ap[3].stride, g_ap[3].ptr); }
    CALL(glBindBuffer, 0x8892, g_arr_buf); CALL(glColor4ub, g_col[0], g_col[1], g_col[2], g_col[3]);
    fprintf(stderr, "[depthtest] run %d at frame %u (red = far quad rejected by depth, green = not rejected):\n", runs, g_frames);
    for (int t = 0; t < 5; t++) fprintf(stderr, "[depthtest]   %-40s -> overlap %u,%u,%u | near-only %u,%u,%u | far-only %u,%u,%u\n", names[t], px[t][0], px[t][1], px[t][2], px[t][4], px[t][5], px[t][6], px[t][8], px[t][9], px[t][10]);
    fflush(stderr);
}

/* ---- capture-frame probes (L+R): pixel colours at fixed scene points after every scene draw from the first lit draw on, and a depth probe on the hero ---- */
static uint8_t g_probe_px[480 * 320 * 4];
static const int g_pp[6][2] = { {240, 140}, {240, 190}, {215, 60}, {265, 110}, {240, 290}, {60, 160} };   /* scene FBO coords (bottom-left origin): 4 on the hero, sky, left wall */
static void probe_read(char *out, size_t n) {
    glReadPixels(0, 0, 480, 320, 0x1908, 0x1401, g_probe_px);
    size_t o = 0;
    for (int k = 0; k < 6; k++) { const uint8_t *c = g_probe_px + ((size_t)g_pp[k][1] * 480 + (size_t)g_pp[k][0]) * 4;
        o += (size_t)snprintf(out + o, n - o, " P%d=%u,%u,%u,%u", k, c[0], c[1], c[2], c[3]); if (o >= n) break; }
}
static void probe_after(void) {
    if (!g_dd_on || !g_dd_fp || !glReadPixels || fbt_get(g_cur_fb) != 1) return;
    int id = g_dd_n - 1;
    if (!g_dprobe_done) return;   /* no readbacks before the depth sweep: each readback restarts the GPU scene */
    if (g_probe_from < 0) g_probe_from = id;
    char b[256]; probe_read(b, sizeof b); fprintf(g_dd_fp, "p %d%s\n", id, b);
}
/* before the first effect draw of the capture frame: depth sweep. 9 grid points (scene FBO coords, bottom-left origin), small quads at NDC z 0.999, 0.985, 0.95, 0.90, 0.30
   (far to near, each a different colour). A quad shows only where the stored depth is farther than it, so the step at which a point changes colour brackets the stored depth there.
   Also an orientation check: a white quad near the NDC top-left corner, then which readback row is white. */

/* ---- depth-lifetime markers (L+R capture frame, scene framebuffer only, NO mid-frame readbacks) ----
   At 6 moments a tiny quad is drawn at NDC z 0.2 WITH depth write (func ALWAYS) at its own spot near the top-left.
   When the game leaves the scene framebuffer, a magenta quad at z 0.5 (depth test LEQUAL) is drawn over all spots and the frame is read once.
   Spot still its own colour = depth from that moment survived to the end; magenta = depth written at that moment was LOST later. */
static int g_mk_done[6], g_mk_at[6], g_mk_fb1n, g_mk_seen_lit, g_mk_seen_fx, g_mk_after_fx;
static const char *g_mk_name[6] = { "frame start (1st scene draw)", "after ~67 draws (sky)", "after 2000 draws (mid world)", "before 1st lit draw (after world)", "before 1st effect draw (after hero)", "after effects (1st draw after them)" };
static void mk_quad(float cx, float cy, float z, uint8_t r, uint8_t g, uint8_t b, unsigned func, int mask) {
    float dx = 5.f / 240.f, dy = 5.f / 160.f, q[12] = { cx - dx, cy - dy, z, cx + dx, cy - dy, z, cx - dx, cy + dy, z, cx + dx, cy + dy, z };
    CALL(glDisable, 0xBE2); CALL(glDisable, 0xB50); CALL(glDisable, 0xB60); CALL(glDisable, 0xDE1); CALL(glDisable, 0xBC0); CALL(glDisable, 0xB44); CALL(glDisable, 0xB57); CALL(glDisable, 0xC11);
    CALL(glEnable, 0xB71); CALL(glDepthFunc, func); CALL(glDepthMask, (unsigned char)mask); CALL(glColorMask, 1, 1, 1, 1);
    CALL(glMatrixMode, 0x1701); CALL(glPushMatrix); CALL(glLoadIdentity); CALL(glMatrixMode, 0x1700); CALL(glPushMatrix); CALL(glLoadIdentity);
    CALL(glBindBuffer, 0x8892, 0); CALL(glEnableClientState, 0x8074); CALL(glDisableClientState, 0x8076); CALL(glDisableClientState, 0x8078); CALL(glDisableClientState, 0x8075);
    CALL(glColor4ub, r, g, b, 255); CALL(glVertexPointer, 3, 0x1406, 0, q); CALL(glDrawArrays, 5, 0, 4);
    CALL(glMatrixMode, 0x1701); CALL(glPopMatrix); CALL(glMatrixMode, 0x1700); CALL(glPopMatrix); CALL(glMatrixMode, g_matmode);
    static const unsigned caps[9] = { 0xB44, 0xBE2, 0xB71, 0xBC0, 0xB50, 0xB60, 0xDE1, 0xB57, 0xC11 }; static const int bits[9] = { 0, 1, 2, 3, 4, 5, 6, 7, 11 };
    for (int i = 0; i < 9; i++) { if ((g_enbits >> bits[i]) & 1) CALL(glEnable, caps[i]); else CALL(glDisable, caps[i]); }
    CALL(glDepthMask, g_dmask); CALL(glDepthFunc, g_dfunc); CALL(glColor4ub, g_col[0], g_col[1], g_col[2], g_col[3]); CALL(glColorMask, g_cm & 1, (g_cm >> 1) & 1, (g_cm >> 2) & 1, (g_cm >> 3) & 1);
    static const unsigned cs[4] = { 0x8074, 0x8075, 0x8076, 0x8078 }; static const int cb[4] = { 0, 1, 2, 4 };
    for (int i = 0; i < 4; i++) { if ((g_clbits >> cb[i]) & 1) CALL(glEnableClientState, cs[i]); else CALL(glDisableClientState, cs[i]); }
    if (g_clbits & 1) { CALL(glBindBuffer, 0x8892, g_ap[0].buf); CALL(glVertexPointer, g_ap[0].size, g_ap[0].type, g_ap[0].stride, g_ap[0].ptr); }
    if (g_clbits & 2) { CALL(glBindBuffer, 0x8892, g_ap[1].buf); CALL(glNormalPointer, g_ap[1].type, g_ap[1].stride, g_ap[1].ptr); }
    if (g_clbits & 4) { CALL(glBindBuffer, 0x8892, g_ap[2].buf); CALL(glColorPointer, g_ap[2].size, g_ap[2].type, g_ap[2].stride, g_ap[2].ptr); }
    if (g_clbits & 16) { CALL(glBindBuffer, 0x8892, g_ap[3].buf); CALL(glTexCoordPointer, g_ap[3].size, g_ap[3].type, g_ap[3].stride, g_ap[3].ptr); }
    CALL(glBindBuffer, 0x8892, g_arr_buf);
}
static float mk_x(int i) { return (30.f + 30.f * i) / 240.f - 1.f; }
static const float g_mk_y = 300.f / 160.f - 1.f;
static void mk_place(int i) {
    static const uint8_t c[6][3] = { {255,0,0}, {0,255,0}, {0,0,255}, {255,255,0}, {0,255,255}, {255,128,0} };
    if (g_mk_done[i]) return; g_mk_done[i] = 1; g_mk_at[i] = g_dd_n - 1;
    mk_quad(mk_x(i), g_mk_y, 0.2f, c[i][0], c[i][1], c[i][2], 0x207, 1);
}
/* called before every draw in the capture frame */
static void mk_before_draw(void) {
    if (!g_dd_on || !g_dd_fp || fbt_get(g_cur_fb) != 1) return;
    g_mk_fb1n++;
    if (g_mk_fb1n == 1) mk_place(0);
    if (g_mk_fb1n == 68) mk_place(1);
    if (g_mk_fb1n == 2001) mk_place(2);
    if (!g_mk_seen_lit && dbg_lit()) { g_mk_seen_lit = 1; mk_place(3); }
    if (!g_mk_seen_fx && dbg_trans()) { g_mk_seen_fx = 1; mk_place(4); }
    if (g_mk_seen_fx && !dbg_trans() && !g_mk_after_fx) { g_mk_after_fx = 1; mk_place(5); }
}
/* called when the game is about to leave the scene framebuffer */
static void mk_check(void) {
    if (!g_dd_on || !g_dd_fp || fbt_get(g_cur_fb) != 1 || !g_mk_done[0] || !glReadPixels) return;
    for (int i = 0; i < 6; i++) if (g_mk_done[i]) mk_quad(mk_x(i), g_mk_y, 0.5f, 255, 0, 255, 0x203, 0);
    glReadPixels(0, 0, 480, 320, 0x1908, 0x1401, g_probe_px);
    fprintf(g_dd_fp, "markers (scene draws seen=%d):\n", g_mk_fb1n);
    for (int i = 0; i < 6; i++) {
        if (!g_mk_done[i]) { fprintf(g_dd_fp, "  marker %d %-40s not placed\n", i, g_mk_name[i]); continue; }
        int px = 30 + 30 * i; const uint8_t *a = g_probe_px + ((size_t)300 * 480 + (size_t)px) * 4, *b = g_probe_px + ((size_t)19 * 480 + (size_t)px) * 4;
        int mag_a = a[0] > 200 && a[1] < 60 && a[2] > 200, mag_b = b[0] > 200 && b[1] < 60 && b[2] > 200;
        fprintf(g_dd_fp, "  marker %d %-40s (at draw %d): pixel %u,%u,%u (flipped row %u,%u,%u) -> %s\n", i, g_mk_name[i], g_mk_at[i], a[0], a[1], a[2], b[0], b[1], b[2],
                (mag_a || mag_b) ? "DEPTH LOST after this point" : "depth survived to the end");
    }
    g_mk_fb1n = -100000;   /* only once */
}
static void mk_reset(void) { memset(g_mk_done, 0, sizeof g_mk_done); g_mk_fb1n = 0; g_mk_seen_lit = g_mk_seen_fx = g_mk_after_fx = 0; }

/* capture frame: when the game leaves a framebuffer, read its centre and two other points (end of a pass: one readback per pass) */
static void pass_probe(void) {
    if (!g_dd_on || !g_dd_fp || !glReadPixels || g_default_fb_bound || !g_cur_fb) return;
    unsigned ft = fbt_get(g_cur_fb); int w = (ft == 2 || ft == 3) ? 120 : 480, h = (ft == 2 || ft == 3) ? 80 : 320;
    char b[256]; size_t o = 0;
    static const float pts[3][2] = { {0.5f, 0.5f}, {0.3f, 0.3f}, {0.7f, 0.7f} };
    glReadPixels(0, 0, w, h, 0x1908, 0x1401, g_probe_px);
    for (int k = 0; k < 3; k++) { int x = (int)(pts[k][0] * w), y = (int)(pts[k][1] * h);
        const uint8_t *c = g_probe_px + ((size_t)y * (size_t)w + (size_t)x) * 4;
        o += (size_t)snprintf(b + o, sizeof b - o, " P%d=%u,%u,%u", k, c[0], c[1], c[2]); }
    fprintf(g_dd_fp, "pass end fbtex=%u (%dx%d):%s\n", ft, w, h, b);
}
static void depth_probe(void) {
    if (1) return;   /* disabled in this build: mid-frame readbacks */
    if (!g_dd_on || !g_dd_fp || g_dprobe_done || !glReadPixels || !dbg_trans()) return;
    g_dprobe_done = 1;
    static const int gx[3] = { 100, 240, 380 }, gy[3] = { 40, 100, 170 };
    static const float zs[5] = { 0.999f, 0.985f, 0.95f, 0.90f, 0.30f };
    static const uint8_t col[5][3] = { {255,0,0}, {0,255,0}, {0,0,255}, {255,255,0}, {0,255,255} };
    uint8_t before[9][4], got[5][9][4];
    memset(before, 0, sizeof before);   /* no readback before the first step (a readback restarts the GPU scene and can drop depth) */
    CALL(glDisable, 0xBE2); CALL(glDisable, 0xB50); CALL(glDisable, 0xB60); CALL(glDisable, 0xDE1); CALL(glDisable, 0xBC0); CALL(glDisable, 0xB44); CALL(glDisable, 0xB57);
    CALL(glEnable, 0xB71); CALL(glDepthFunc, 0x203); CALL(glDepthMask, 0);
    CALL(glMatrixMode, 0x1701); CALL(glPushMatrix); CALL(glLoadIdentity); CALL(glMatrixMode, 0x1700); CALL(glPushMatrix); CALL(glLoadIdentity);
    CALL(glBindBuffer, 0x8892, 0); CALL(glEnableClientState, 0x8074); CALL(glDisableClientState, 0x8076); CALL(glDisableClientState, 0x8078); CALL(glDisableClientState, 0x8075);
    for (int z = 0; z < 5; z++) {
        CALL(glColor4ub, col[z][0], col[z][1], col[z][2], 255);
        for (int k = 0; k < 9; k++) {
            float cx = gx[k % 3] / 240.f - 1.f, cy = gy[k / 3] / 160.f - 1.f, dx = 3.f / 240.f, dy = 3.f / 160.f;
            float q[12] = { cx - dx, cy - dy, zs[z], cx + dx, cy - dy, zs[z], cx - dx, cy + dy, zs[z], cx + dx, cy + dy, zs[z] };
            CALL(glVertexPointer, 3, 0x1406, 0, q); CALL(glDrawArrays, 5, 0, 4);
        }
        glReadPixels(0, 0, 480, 320, 0x1908, 0x1401, g_probe_px);
        for (int k = 0; k < 9; k++) memcpy(got[z][k], g_probe_px + ((size_t)gy[k / 3] * 480 + (size_t)gx[k % 3]) * 4, 4);
    }
    /* orientation: white quad at NDC x -0.95..-0.85, y 0.85..0.95 (top-left) with depth test off */
    CALL(glDisable, 0xB71); CALL(glColor4ub, 255, 255, 255, 255);
    { float q[12] = { -0.95f, 0.85f, 0, -0.85f, 0.85f, 0, -0.95f, 0.95f, 0, -0.85f, 0.95f, 0 }; CALL(glVertexPointer, 3, 0x1406, 0, q); CALL(glDrawArrays, 5, 0, 4); }
    glReadPixels(0, 0, 480, 320, 0x1908, 0x1401, g_probe_px);
    const uint8_t *top = g_probe_px + ((size_t)288 * 480 + 24) * 4, *bot = g_probe_px + ((size_t)31 * 480 + 24) * 4;
    fprintf(g_dd_fp, "orient: readback row 288 (top if bottom-left origin) = %u,%u,%u | row 31 = %u,%u,%u -> %s\n", top[0], top[1], top[2], bot[0], bot[1], bot[2],
            top[0] > 240 && top[1] > 240 && top[2] > 240 ? "bottom-left origin (GL standard)" : bot[0] > 240 && bot[1] > 240 && bot[2] > 240 ? "TOP-left origin (flipped)" : "unclear");
    for (int k = 0; k < 9; k++) {
        char pl[64] = ""; int farthest = -1;
        for (int z = 0; z < 5; z++) { int ok = got[z][k][0] == col[z][0] && got[z][k][1] == col[z][1] && got[z][k][2] == col[z][2];
            if (ok) { char t[12]; snprintf(t, sizeof t, "%.3f ", (double)zs[z]); strcat(pl, t); if (farthest < 0) farthest = z; } }
        fprintf(g_dd_fp, "dsweep point (%d,%d) FIRST STEP (clean) z0.999 quad %s | later steps (may be affected by readbacks) passed: %s| %s\n", gx[k % 3], gy[k / 3], (got[0][k][0] == 255 && got[0][k][1] == 0 && got[0][k][2] == 0) ? "DREW (depth there >= 0.999: empty)" : "HIDDEN (depth works here)", pl[0] ? pl : "none ",
                farthest < 0 ? "nearer than 0.30 (or quads not drawn)" : farthest == 0 ? "~1.0 cleared" : farthest == 1 ? "0.985-0.999" : farthest == 2 ? "0.95-0.985" : farthest == 3 ? "0.90-0.95" : "0.30-0.90");
    }
    /* restore */
    CALL(glMatrixMode, 0x1701); CALL(glPopMatrix); CALL(glMatrixMode, 0x1700); CALL(glPopMatrix); CALL(glMatrixMode, g_matmode);
    static const unsigned caps[8] = { 0xB44, 0xBE2, 0xB71, 0xBC0, 0xB50, 0xB60, 0xDE1, 0xB57 }; static const int bits[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };
    for (int i = 0; i < 8; i++) { if ((g_enbits >> bits[i]) & 1) CALL(glEnable, caps[i]); else CALL(glDisable, caps[i]); }
    CALL(glDepthMask, g_dmask); CALL(glDepthFunc, g_dfunc); CALL(glColor4ub, g_col[0], g_col[1], g_col[2], g_col[3]);
    static const unsigned cs[4] = { 0x8074, 0x8075, 0x8076, 0x8078 }; static const int cb[4] = { 0, 1, 2, 4 };
    for (int i = 0; i < 4; i++) { if ((g_clbits >> cb[i]) & 1) CALL(glEnableClientState, cs[i]); else CALL(glDisableClientState, cs[i]); }
    if (g_clbits & 1) { CALL(glBindBuffer, 0x8892, g_ap[0].buf); CALL(glVertexPointer, g_ap[0].size, g_ap[0].type, g_ap[0].stride, g_ap[0].ptr); }
    if (g_clbits & 2) { CALL(glBindBuffer, 0x8892, g_ap[1].buf); CALL(glNormalPointer, g_ap[1].type, g_ap[1].stride, g_ap[1].ptr); }
    if (g_clbits & 4) { CALL(glBindBuffer, 0x8892, g_ap[2].buf); CALL(glColorPointer, g_ap[2].size, g_ap[2].type, g_ap[2].stride, g_ap[2].ptr); }
    if (g_clbits & 16) { CALL(glBindBuffer, 0x8892, g_ap[3].buf); CALL(glTexCoordPointer, g_ap[3].size, g_ap[3].type, g_ap[3].stride, g_ap[3].ptr); }
    CALL(glBindBuffer, 0x8892, g_arr_buf);
}

/* bloom composite: the game adds the blur texture (tex 2) onto the final framebuffer (fbtex 4) with blend SRC_ALPHA,ONE and colour alpha 102.
   vitaGL adds it at full strength (the alpha weight is lost), so it is drawn instead with blend ONE,ONE and a grey colour = alpha * bloom (same result, no alpha needed).
   Mode 1 (= game as-is) skips this. In a capture the final image is read before and after. */
static int g_comp;
static void comp_probe(const char *tag) {
    if (!g_dd_on || !g_dd_fp || !glReadPixels) return;
    glReadPixels(0, 0, 480, 320, 0x1908, 0x1401, g_probe_px);
    static const float pts[3][2] = { {0.5f, 0.5f}, {0.3f, 0.3f}, {0.7f, 0.7f} }; char b[200]; size_t o = 0;
    for (int k = 0; k < 3; k++) { const uint8_t *c = g_probe_px + ((size_t)(pts[k][1] * 320) * 480 + (size_t)(pts[k][0] * 480)) * 4; o += (size_t)snprintf(b + o, sizeof b - o, " P%d=%u,%u,%u", k, c[0], c[1], c[2]); }
    fprintf(g_dd_fp, "composite %s:%s\n", tag, b);
}
static void comp_pre(void) {
    g_comp = 0;
    if (fbt_get(g_cur_fb) != 4 || g_bound_tex != 2 || g_bs != 0x302 || g_bd != 1 || !((g_enbits >> 1) & 1)) return;
    comp_probe("before bloom add");
    if (g_dbgmode == 1) { g_comp = 2; return; }
    float k = (float)g_col[3] / 255.f * g_bloom; if (k > 1.f) k = 1.f; unsigned gv = (unsigned)(k * 255.f + 0.5f);
    CALL(glBlendFunc, 1, 1); CALL(glColor4ub, gv, gv, gv, 255); g_comp = 1;
    static int once; if (!once) { once = 1; fprintf(stderr, "[gl] bloom composite drawn as ONE,ONE with grey %u (alpha %u x bloom %.2f)\n", gv, g_col[3], (double)g_bloom); fflush(stderr); }
}
static void comp_post(void) {
    if (!g_comp) return;
    if (g_comp == 1) { CALL(glBlendFunc, g_bs, g_bd); CALL(glColor4ub, g_col[0], g_col[1], g_col[2], g_col[3]); }
    comp_probe("after bloom add");
    g_comp = 0;
}
static void w_glDrawArrays_inner(unsigned m, int f, int c);
static __attribute__((used)) void w_glDrawArrays(unsigned m, int f, int c) { unsigned long long t0 = prof_now(); w_glDrawArrays_inner(m, f, c); g_pf_draw += prof_now() - t0; }
static void w_glDrawArrays_inner(unsigned m, int f, int c) { ENTER("glDrawArrays", glDrawArrays); if (g_turbo) return; reupload_dyn(); dd_record("A", m, c, 0); dd_geom(1, f, c, 0, NULL); ULOG("drawArrays mode=0x%x", m); g_st_draws++; g_ch[g_cull_en ? 1 : 0][g_cull_mode == 0x404][g_front == 0x900]++; ARGS(m,c); note_draw(); g_draws++; g_skip_first = f; g_skip_cnt = c; g_skip_it = 0; g_skip_ip = NULL; depth_probe(); mk_before_draw(); if (dbg_skip(1)) return; dbg_pre(1, f, c, 0, NULL); cm_pre(); comp_pre(); if (!fx_try_clip(1, m, f, c, 0, NULL) && !cpu_light_draw(m, f, c)) CALL(glDrawArrays, m, f, c); g_pf_hw++; comp_post(); cm_post(); dbg_post(); probe_after(); depth_selftest2(); }
static void w_glDrawElements_inner(unsigned m, int c, unsigned t, const void *i);
static __attribute__((used)) void w_glDrawElements(unsigned m, int c, unsigned t, const void *i) { unsigned long long t0 = prof_now(); w_glDrawElements_inner(m, c, t, i); g_pf_draw += prof_now() - t0; }
static int batch_add(unsigned m, int c, unsigned t, const void *i);
static void w_glDrawElements_inner(unsigned m, int c, unsigned t, const void *i) { ENTER(g_name_de, glDrawElements); if (g_turbo) return; if (batch_add(m, c, t, i)) { g_st_draws++; g_draws++; note_draw(); return; } reupload_dyn(); dd_record("E", m, c, t); dd_geom(0, 0, c, t, i); ULOG("drawElements mode=0x%x type=0x%x", m, t); g_st_draws++; g_ch[g_cull_en ? 1 : 0][g_cull_mode == 0x404][g_front == 0x900]++; ARGS(m,c); note_draw(); g_draws++; g_skip_first = 0; g_skip_cnt = c; g_skip_it = t; g_skip_ip = i; depth_probe(); mk_before_draw(); if (dbg_skip(0)) return; { const void *i0 = i; dbg_pre(0, 0, c, t, &i); cm_pre(); if (!fx_try_clip(0, m, 0, c, t, i0)) { CALL(glDrawElements, m, c, t, i); g_pf_hw++; } cm_post(); } dbg_post(); probe_after(); depth_selftest2(); }
static void batch_flush(void) {
    if (!g_bq_n) return;
    int n = g_bq_n; g_bq_n = 0;
    CALL(glBindBuffer, 0x8893, 0); CALL(glDrawElements, 4, n, 0x1403, g_bq); CALL(glBindBuffer, 0x8893, g_elem_buf); g_pf_hw++;
}
static int batch_add(unsigned m, int c, unsigned t, const void *i) {
    if (!g_batch || g_dd_on || g_dbgmode || (m != 5 && m != 4) || t != 0x1403 || c < 3 || !g_elem_buf) return 0;
    if (!(g_clbits & 1) || !g_ap[0].buf || ((g_clbits & 2) && !g_ap[1].buf) || ((g_clbits & 4) && !g_ap[2].buf) || ((g_clbits & 16) && !g_ap[3].buf)) return 0;   /* only buffer-backed vertex data */
    if ((g_enbits & ((1u << 4) | (1u << 10))) == ((1u << 4) | (1u << 10))) return 0;    /* lit draws go through the CPU lighting path */
    Shad *es = shad_find(g_elem_buf); size_t off = (size_t)(uintptr_t)i;
    if (!es || off + (size_t)c * 2 > (size_t)es->size) return 0;
    const uint16_t *ix = (const uint16_t *)(es->d + off);
    int need = m == 4 ? c : (c - 2) * 3;
    if (g_bq_n + need > g_bq_cap) { batch_flush(); if (need > g_bq_cap) { int cap = need > 65536 ? need : 65536; uint16_t *nb = realloc(g_bq, (size_t)cap * 2); if (!nb) return 0; g_bq = nb; g_bq_cap = cap; } }
    uint16_t *o = g_bq + g_bq_n;
    if (m == 4) { memcpy(o, ix, (size_t)c * 2); o += c; }
    else for (int k = 0; k + 2 < c; k++) {                    /* strip -> list, keeping GL's alternating winding; degenerate joins dropped */
        uint16_t a = ix[k], b = ix[k + 1], d = ix[k + 2];
        if (a == b || b == d || a == d) continue;
        if (k & 1) { *o++ = b; *o++ = a; *o++ = d; } else { *o++ = a; *o++ = b; *o++ = d; }
    }
    g_bq_n = (int)(o - g_bq); g_pf_batched++;
    return 1;
}

/* ---------------- buffers / textures ---------------- */
static __attribute__((used)) void w_glGenBuffers(int n, unsigned *ids) { ENTER("glGenBuffers", glGenBuffers); CALL(glGenBuffers, n, ids); }
static __attribute__((used)) void w_glBindBuffer(unsigned t, unsigned b) { ENTER("glBindBuffer", glBindBuffer); ARGS(t,b); if (t == 0x8892) g_arr_buf = b; else if (t == 0x8893) g_elem_buf = b; CALL(glBindBuffer, t, b); }
static void w_glBufferData_inner(unsigned t, int sz, const void *d, unsigned u);
static __attribute__((used)) void w_glBufferData(unsigned t, int sz, const void *d, unsigned u) { unsigned long long t0 = prof_now(); w_glBufferData_inner(t, sz, d, u); g_pf_upload += prof_now() - t0; }
static void w_glBufferData_inner(unsigned t, int sz, const void *d, unsigned u) { ENTER("glBufferData", glBufferData);
    if ((t == 0x8892 || (t == 0x8893 && (g_debugkeys || g_batch))) && g_cpulight && BUFID(t)) { Shad *sh = shad_find(BUFID(t)); if (sh && sh->size > sz) { g_sh_total -= (unsigned)(sh->size - sz); sh->size = sz; } sh = shad_get(BUFID(t), sz); if (sh) { sh->nbd++; sh->nsub = 0; sh->sub_bytes = 0; if (d && sz > 0) memcpy(sh->d, d, (size_t)sz); else if (sz > 0) memset(sh->d, 0, (size_t)sz); } } CALL(glBufferData, t, sz, d, u); }
static void w_glBufferSubData_inner(unsigned t, int o, int sz, const void *d);
static __attribute__((used)) void w_glBufferSubData(unsigned t, int o, int sz, const void *d) { unsigned long long t0 = prof_now(); w_glBufferSubData_inner(t, o, sz, d); g_pf_upload += prof_now() - t0; }
static void w_glBufferSubData_inner(unsigned t, int o, int sz, const void *d) { ENTER("glBufferSubData", glBufferSubData);
    if ((t == 0x8892 || (t == 0x8893 && (g_debugkeys || g_batch))) && g_cpulight && BUFID(t) && d && sz > 0 && o >= 0) { Shad *sh = shad_get(BUFID(t), o + sz); if (sh) { memcpy(sh->d + o, d, (size_t)sz); sh->nsub++; sh->sub_bytes += (unsigned)sz; sh->lastsrc = d; sh->lastoff = o; sh->lastsz = sz; } } CALL(glBufferSubData, t, o, sz, d); }
static __attribute__((used)) void w_glDeleteBuffers(int n, const unsigned *ids) { ENTER("glDeleteBuffers", glDeleteBuffers);
    for (int i = 0; i < n; i++) { Shad *sh = shad_find(ids[i]); if (sh) { free(sh->d); g_sh_total -= (unsigned)sh->size; *sh = g_sh[--g_sh_n]; } } CALL(glDeleteBuffers, n, ids); }
static __attribute__((used)) void w_glGenTextures(int n, unsigned *ids) { ENTER("glGenTextures", glGenTextures); CALL(glGenTextures, n, ids); }
static __attribute__((used)) void w_glBindTexture(unsigned t, unsigned id) { ENTER("glBindTexture", glBindTexture); ARGS(t,id); if (g_sfilter && t == 0xDE1 && g_u_tex[g_act_unit] == id) { g_bound_tex = id; g_pf_skips++; return; } g_bound_tex = id; g_u_tex[g_act_unit] = id; CALL(glBindTexture, t, id); }
static __attribute__((used)) void w_glDeleteTextures(int n, const unsigned *ids) { ENTER("glDeleteTextures", glDeleteTextures);
    for (int i = 0; i < n; i++) for (int u = 0; u < 4; u++) if (g_u_tex[u] == ids[i]) { g_u_tex[u] = 0; if (g_bound_tex == ids[i]) g_bound_tex = 0; }   /* GL unbinds deleted textures */
    CALL(glDeleteTextures, n, ids); }
static __attribute__((used)) void SOFT w_glTexParameterf(unsigned t, unsigned p, float v) { ENTER("glTexParameterf", glTexParameterf); ULOG("texParamf 0x%x 0x%x %d", t, p, (int)v); ARGS(p,(int)v); CALL(glTexParameterf, t, p, v); }
static __attribute__((used)) void w_glTexParameteri(unsigned t, unsigned p, int v) { ENTER("glTexParameteri", glTexParameteri); ULOG("texParam 0x%x 0x%x %d", t, p, v); ARGS(p,v); CALL(glTexParameteri, t, p, v); }
/* 16-bit packed pixels (4-4-4-4 / 5-5-5-1) -> RGBA8888, so vitaGL's channel order for those formats cannot matter */
static unsigned char *conv16(const void *d, int w, int h, unsigned type) {
    unsigned char *o = malloc((size_t)w * h * 4); if (!o) return NULL;
    size_t stride = (((size_t)w * 2) + 3) & ~(size_t)3;
    for (int y = 0; y < h; y++) {
        const unsigned char *row = (const unsigned char *)d + stride * y;
        for (int x = 0; x < w; x++) {
            unsigned v = row[x * 2] | (row[x * 2 + 1] << 8); unsigned char *p = o + ((size_t)y * w + x) * 4;
            if (type == 0x8033) { p[0] = ((v >> 12) & 15) * 17; p[1] = ((v >> 8) & 15) * 17; p[2] = ((v >> 4) & 15) * 17; p[3] = (v & 15) * 17; }
            else { p[0] = ((v >> 11) & 31) * 255 / 31; p[1] = ((v >> 6) & 31) * 255 / 31; p[2] = ((v >> 1) & 31) * 255 / 31; p[3] = (v & 1) ? 255 : 0; }
        }
    }
    return o;
}
static void w_glTexImage2D_inner(unsigned t, int l, int ifmt, int w, int h, int b, unsigned f, unsigned ty, const void *d);
static __attribute__((used)) void w_glTexImage2D(unsigned t, int l, int ifmt, int w, int h, int b, unsigned f, unsigned ty, const void *d) { unsigned long long t0 = prof_now(); w_glTexImage2D_inner(t, l, ifmt, w, h, b, f, ty, d); g_pf_tex += prof_now() - t0; }
static void w_glTexImage2D_inner(unsigned t, int l, int ifmt, int w, int h, int b, unsigned f, unsigned ty, const void *d) {
    ENTER("glTexImage2D", glTexImage2D);
    ULOG("texImage lvl=%d ifmt=0x%x fmt=0x%x type=0x%x %dx%d data=%s", l, ifmt, f, ty, w, h, d ? "y" : "n"); g_st_texup++; g_st_texpix += (unsigned)(w*h);
    if (g_fixfmt && d && f == 0x1908 && (ty == 0x8033 || ty == 0x8034)) {
        unsigned char *px = conv16(d, w, h, ty);
        if (px) { if (l == 0) { tinfo_set(g_bound_tex, w, 2, px, w * h); tex_dump(0, "rgba", g_bound_tex, w, h, px, (size_t)w * h * 4); } CALL(glTexImage2D, t, l, 0x1908, w, h, b, 0x1908, 0x1401, px); free(px); return; }
    }
    if (d && l == 0 && f == 0x1908 && ty == 0x1401) tinfo_set(g_bound_tex, w, 1, (const unsigned char *)d, w * h);
    if (d && l == 0 && f == 0x1908 && ty == 0x1401 && w >= 64) tex_dump(1, "rgba", g_bound_tex, w, h, d, (size_t)w * h * 4);
    CALL(glTexImage2D, t, l, ifmt, w, h, b, f, ty, d);
}
static __attribute__((used)) void w_glTexSubImage2D(unsigned t, int l, int x, int y, int w, int h, unsigned f, unsigned ty, const void *d) { ENTER("glTexSubImage2D", glTexSubImage2D); ULOG("texSub fmt=0x%x type=0x%x %dx%d", f, ty, w, h); g_st_texup++; g_st_texpix += (unsigned)(w*h);
    if (g_fixfmt && d && f == 0x1908 && (ty == 0x8033 || ty == 0x8034)) {
        unsigned char *px = conv16(d, w, h, ty);
        if (px) { CALL(glTexSubImage2D, t, l, x, y, w, h, 0x1908, 0x1401, px); free(px); return; }
    }
    CALL(glTexSubImage2D, t, l, x, y, w, h, f, ty, d); }
static void w_glCompressedTexImage2D_inner(unsigned t, int l, unsigned f, int w, int h, int b, int sz, const void *d);
static __attribute__((used)) void w_glCompressedTexImage2D(unsigned t, int l, unsigned f, int w, int h, int b, int sz, const void *d) { unsigned long long t0 = prof_now(); w_glCompressedTexImage2D_inner(t, l, f, w, h, b, sz, d); g_pf_tex += prof_now() - t0; }
static void w_glCompressedTexImage2D_inner(unsigned t, int l, unsigned f, int w, int h, int b, int sz, const void *d) {
    ENTER("glCompressedTexImage2D", glCompressedTexImage2D);
    ULOG("compressedTex lvl=%d fmt=0x%x %dx%d size=%d", l, f, w, h, sz); g_st_texup++;
    if (l == 0 && d) { char k[16]; snprintf(k, sizeof k, "pvr%04x", f); tex_dump(2, k, g_bound_tex, w, h, d, (size_t)sz); }
    if (g_texdiag && l == 0 && d && f >= 0x8C00 && f <= 0x8C03) { unsigned char *tmp = malloc((size_t)w * h * 4); if (tmp) { int bpp2 = (f == 0x8C01 || f == 0x8C03), al = (f == 0x8C02 || f == 0x8C03);
        if (pvrtc_decode((const unsigned char *)d, w, h, bpp2, al, g_pvrtc_var, tmp) == 0) tinfo_set(g_bound_tex, w, 3, tmp, w * h); free(tmp); } }
    if (g_swpvrtc && d && f >= 0x8C00 && f <= 0x8C03) {
        int bpp2 = (f == 0x8C01 || f == 0x8C03), alpha = (f == 0x8C02 || f == 0x8C03);
        unsigned char *px = malloc((size_t)w * h * 4);
        if (px && pvrtc_decode((const unsigned char *)d, w, h, bpp2, alpha, g_pvrtc_var, px) == 0) {
            CALL(glTexImage2D, t, l, 0x1908, w, h, 0, 0x1908, 0x1401, px); free(px); return;
        }
        free(px); ULOG("swpvrtc: decode failed for %dx%d, using hardware path", w, h);
    }
    CALL(glCompressedTexImage2D, t, l, f, w, h, b, sz, d);
}
static __attribute__((used)) void w_glCompressedTexSubImage2D(unsigned t, int l, int x, int y, int w, int h, unsigned f, int sz, const void *d) { ENTER("glCompressedTexSubImage2D", glCompressedTexSubImage2D); ULOG("compressedSub fmt=0x%x %dx%d", f, w, h); g_st_texup++; CALL(glCompressedTexSubImage2D, t, l, x, y, w, h, f, sz, d); }
static __attribute__((used)) void w_glCopyTexImage2D(unsigned t, int l, unsigned f, int x, int y, int w, int h, int b) { ENTER("glCopyTexImage2D", glCopyTexImage2D); CALL(glCopyTexImage2D, t, l, f, x, y, w, h, b); }
static __attribute__((used)) void w_glGenerateMipmapOES(unsigned t) { ENTER("glGenerateMipmapOES", glGenerateMipmap); CALL(glGenerateMipmap, t); }
static __attribute__((used)) void w_glReadPixels(int x, int y, int w, int h, unsigned f, unsigned t, void *d) { ENTER("glReadPixels", glReadPixels); CALL(glReadPixels, x, y, w, h, f, t, d); }

/* ---------------- framebuffer objects ---------------- */
static __attribute__((used)) void w_glGenFramebuffersOES(int n, unsigned *ids) { ENTER("glGenFramebuffersOES", glGenFramebuffers); CALL(glGenFramebuffers, n, ids); for (int i = 0; i < n; i++) FBLOG("GenFramebuffer -> 0x%x", ids[i]); }
static __attribute__((used)) void w_glGenRenderbuffersOES(int n, unsigned *ids) { ENTER("glGenRenderbuffersOES", glGenRenderbuffers); CALL(glGenRenderbuffers, n, ids); for (int i = 0; i < n; i++) FBLOG("GenRenderbuffer -> 0x%x", ids[i]); }
static __attribute__((used)) void w_glDeleteFramebuffersOES(int n, const unsigned *ids) { ENTER("glDeleteFramebuffersOES", glDeleteFramebuffers); CALL(glDeleteFramebuffers, n, ids); }
static __attribute__((used)) void w_glDeleteRenderbuffersOES(int n, const unsigned *ids) { ENTER("glDeleteRenderbuffersOES", glDeleteRenderbuffers); CALL(glDeleteRenderbuffers, n, ids); }
/* one-off self-test: does the scene framebuffer (fbo with colour texture 1 + depth renderbuffer) really depth-test? Draws a near red quad, then a far green quad over it; red in the middle = depth works. */
static __attribute__((unused)) void depth_selftest(void) {
    static int done; if (done) return; done = 1;
    static const float nearq[12] = { -0.5f,-0.5f,-0.5f,  0.5f,-0.5f,-0.5f,  -0.5f,0.5f,-0.5f,  0.5f,0.5f,-0.5f };
    static const float farq[12]  = { -0.3f,-0.3f, 0.5f,  0.7f,-0.3f, 0.5f,  -0.3f,0.7f, 0.5f,  0.7f,0.7f, 0.5f };
    unsigned char mid[4] = {0}, edge[4] = {0};
    CALL(glViewport, 0, 0, 480, 320);
    CALL(glDisable, 0xC11); CALL(glDisable, 0xB44); CALL(glDisable, 0xBE2); CALL(glDisable, 0xBC0); CALL(glDisable, 0xB50); CALL(glDisable, 0xB60); CALL(glDisable, 0xDE1);
    CALL(glEnable, 0xB71); CALL(glDepthFunc, 0x203); CALL(glDepthMask, 1); CALL(glColorMask, 1, 1, 1, 1);
    CALL(glClearColor, 0.f, 0.f, 0.f, 1.f); CALL(glClearDepthf, 1.f); CALL(glClear, 0x4100);
    CALL(glMatrixMode, 0x1701); CALL(glPushMatrix); CALL(glLoadIdentity); CALL(glMatrixMode, 0x1700); CALL(glPushMatrix); CALL(glLoadIdentity);
    CALL(glBindBuffer, 0x8892, 0);
    CALL(glEnableClientState, 0x8074); CALL(glDisableClientState, 0x8075); CALL(glDisableClientState, 0x8076); CALL(glDisableClientState, 0x8078);
    CALL(glVertexPointer, 3, 0x1406, 0, nearq); CALL(glColor4ub, 255, 0, 0, 255); CALL(glDrawArrays, 5, 0, 4);
    CALL(glVertexPointer, 3, 0x1406, 0, farq);  CALL(glColor4ub, 0, 255, 0, 255); CALL(glDrawArrays, 5, 0, 4);
    if (glReadPixels) { glReadPixels(240, 160, 1, 1, 0x1908, 0x1401, mid); glReadPixels(384, 256, 1, 1, 0x1908, 0x1401, edge); }
    CALL(glClear, 0x4100);
    CALL(glDisableClientState, 0x8074); CALL(glMatrixMode, 0x1701); CALL(glPopMatrix); CALL(glMatrixMode, 0x1700); CALL(glPopMatrix);
    CALL(glDisable, 0xB71); CALL(glDepthFunc, 0x201); CALL(glColor4ub, 255, 255, 255, 255); CALL(glBindBuffer, 0x8892, g_arr_buf);
    fprintf(stderr, "[depthtest] scene fbo: overlap pixel=%u,%u,%u (red=depth WORKS, green=NO depth), far-only pixel=%u,%u,%u -> %s\n", mid[0], mid[1], mid[2], edge[0], edge[1], edge[2],
            mid[0] > 200 && mid[1] < 60 ? "DEPTH WORKS" : mid[1] > 200 && mid[0] < 60 ? "NO DEPTH TEST" : "inconclusive"); fflush(stderr);
}
static __attribute__((used)) void w_glBindRenderbufferOES(unsigned t, unsigned rb) {
    ENTER("glBindRenderbufferOES", glBindRenderbuffer); ARGS(t,rb); g_cur_rb = rb;
    if (rb && rb == g_drawable_rb) return;
    CALL(glBindRenderbuffer, t, rb);
}
static __attribute__((used)) void w_glRenderbufferStorageOES(unsigned t, unsigned f, int w, int h) {
    ENTER("glRenderbufferStorageOES", glRenderbufferStorage);
    { static int n; if (n < 6) { n++; fprintf(stderr, "[gl] glRenderbufferStorage rb=%u fmt=0x%x %dx%d\n", g_cur_rb, f, w, h); fflush(stderr); } }
    if (g_cur_rb < 512) { g_rb_w[g_cur_rb] = w; g_rb_h[g_cur_rb] = h; }
    if (g_cur_rb && g_cur_rb == g_drawable_rb) return;
    CALL(glRenderbufferStorage, t, f, w, h);
}
static __attribute__((used)) void w_glBindFramebufferOES(unsigned t, unsigned fb) {
    ENTER("glBindFramebufferOES", glBindFramebuffer); g_st_fbbind++; ARGS(t,fb); if (fb != g_cur_fb) { mk_check(); pass_probe(); } g_cur_fb = fb; if (g_dd_on && g_dd_fp) fprintf(g_dd_fp, "s %d bindfb tex=%u\n", g_dd_n, fbt_get(fb)); FBLOG("BindFramebuffer 0x%x", fb);
    if (fb == 0 || in_list(g_default_fbs, g_n_default_fbs, fb)) { g_default_fb_bound = 1; CALL(glBindFramebuffer, t, win_fb()); }
    else { g_default_fb_bound = 0; CALL(glBindFramebuffer, t, fb); }
    depth_reapply();
}
static void depth_reapply(void) {
    if (g_dbgmode == 1) return;
    if ((g_enbits >> 2) & 1) CALL(glEnable, 0xB71); else CALL(glDisable, 0xB71);
    CALL(glDepthFunc, g_dfunc); CALL(glDepthMask, (unsigned char)g_dmask); g_depth_reapply_n++;
    if (g_dbgmode == 3) return;   /* mode 3: depth only (previous behaviour) */
    /* the same loss affects blend / alpha test / cull / colour mask / fog / texture enables: re-send all tracked fixed-function state */
    static const unsigned caps[8] = { 0xB44, 0xBE2, 0xBC0, 0xB60, 0xDE1, 0xB57, 0xC11, 0xB90 }; static const int bits[8] = { 0, 1, 3, 5, 6, 7, 11, 8 };
    for (int i = 0; i < 8; i++) { if ((caps[i] == 0xB60 && g_nofog) || (caps[i] == 0xB44 && g_cullopt == 1)) continue; if ((g_enbits >> bits[i]) & 1) CALL(glEnable, caps[i]); else CALL(glDisable, caps[i]); }
    CALL(glBlendFunc, g_bs ? g_bs : 1, g_bd); CALL(glBlendEquation, g_beq); CALL(glAlphaFunc, g_afunc, g_aref);
    CALL(glColorMask, g_cm & 1, (g_cm >> 1) & 1, (g_cm >> 2) & 1, (g_cm >> 3) & 1);
    if (g_cull_mode) CALL(glCullFace, g_cull_mode);
    CALL(glColor4ub, g_col[0], g_col[1], g_col[2], g_col[3]);
}
static __attribute__((used)) void w_glFramebufferRenderbufferOES(unsigned t, unsigned att, unsigned rbt, unsigned rb) {
    ENTER("glFramebufferRenderbufferOES", glFramebufferRenderbuffer); FBLOG("fb 0x%x: attach rb 0x%x at 0x%x", g_cur_fb, rb, att);
    if (rb && rb == g_drawable_rb) {                    /* colour buffer of the window: the bound fb IS the window */
        if (g_cur_fb && !in_list(g_default_fbs, g_n_default_fbs, g_cur_fb) && g_n_default_fbs < 16) g_default_fbs[g_n_default_fbs++] = g_cur_fb;
        g_default_fb_bound = 1; CALL(glBindFramebuffer, t, win_fb()); return;
    }
    if (g_default_fb_bound) return;                     /* depth/stencil for the window: vitaGL already has its own */
    CALL(glFramebufferRenderbuffer, t, att, rbt, rb);
}
static __attribute__((used)) void w_glFramebufferTexture2DOES(unsigned t, unsigned att, unsigned tt, unsigned tex, int lvl) { ENTER("glFramebufferTexture2DOES", glFramebufferTexture2D); ARGS(tex,att); FBLOG("fb 0x%x: attach texture %u at 0x%x", g_cur_fb, tex, att); if (att == 0x8CE0) fbt_set(g_cur_fb, tex); CALL(glFramebufferTexture2D, t, att, tt, tex, lvl); }
static __attribute__((used)) void w_glGetRenderbufferParameterivOES(unsigned t, unsigned pname, int *out) {
    ENTER("glGetRenderbufferParameterivOES (emulated)", glGetError); (void)t;
    unsigned rb = g_cur_rb;
    if (rb && rb == g_drawable_rb) *out = (pname == 0x8D42) ? (int)g_game_w : (pname == 0x8D43) ? (int)g_game_h : 0;
    else *out = (rb < 512) ? ((pname == 0x8D42) ? g_rb_w[rb] : (pname == 0x8D43) ? g_rb_h[rb] : 0) : 0;
}

/* ---------------- queries ---------------- */
static __attribute__((used)) const char *w_glGetString(unsigned name) {
    ENTER("glGetString (emulated)", glGetError);
    switch (name) {
        case 0x1F00: return "Apple Computer, Inc.";
        case 0x1F01: return "Apple Software Renderer";
        case 0x1F02: return "OpenGL ES-CM 1.1";
        case 0x1F03: return "GL_OES_blend_subtract GL_OES_framebuffer_object GL_OES_rgb8_rgba8 GL_OES_texture_mirrored_repeat "
                            "GL_OES_packed_depth_stencil GL_IMG_texture_compression_pvrtc GL_APPLE_texture_2D_limited_npot "
                            "GL_APPLE_texture_format_BGRA8888 GL_EXT_texture_filter_anisotropic";
        default: return "";
    }
}
static __attribute__((used)) void w_glGetIntegerv(unsigned pname, int *out) {
    ENTER("glGetIntegerv", glGetIntegerv);
    switch (pname) {
        case 0x0D33: *out = 2048; break;     /* GL_MAX_TEXTURE_SIZE */
        case 0x8842: *out = 9;    break;     /* GL_MAX_PALETTE_MATRICES_OES */
        case 0x86A4: *out = 3;    break;     /* GL_MAX_VERTEX_UNITS_OES */
        case 0x84E2: *out = 2;    break;     /* GL_MAX_TEXTURE_UNITS */
        case 0x8CA6: *out = 0;    break;     /* GL_FRAMEBUFFER_BINDING_OES */
        case 0x8CA7: *out = 0;    break;     /* GL_RENDERBUFFER_BINDING_OES */
        default: *out = 0; if (glGetIntegerv) glGetIntegerv(pname, out); break;
    }
}
static __attribute__((used)) void w_glGetFloatv(unsigned pname, float *out) { ENTER("glGetFloatv", glGetFloatv); if (glGetFloatv) glGetFloatv(pname, out); else *out = 0; }

/* The guest (iOS armv7) only keeps the stack 4-byte aligned, the host ABI (and vitaGL's NEON code, e.g. vld1.64 [sp:64]) needs 8.
 * Every guest->host entry point therefore goes through a trampoline that re-aligns the stack and copies the first 6 stack
 * arguments (r0-r3 pass through untouched). */

/* ---- added for BB3: calls Baroque never made ---- */
static float g_mstack[2][16][16]; static int g_msp[2];
static int mstk(void) { return g_matmode == 0x1700 ? 0 : g_matmode == 0x1701 ? 1 : -1; }
static __attribute__((used)) unsigned w_glCheckFramebufferStatusOES(unsigned t) { (void)t; return 0x8CD5; }   /* COMPLETE */
static __attribute__((used)) unsigned char w_glIsBuffer(unsigned b) { return b != 0; }
static __attribute__((used)) unsigned char w_glIsTexture(unsigned t) { if (glIsTexture) return glIsTexture(t); return t != 0; }
static __attribute__((used)) unsigned char w_glIsEnabled(unsigned c) { if (glIsEnabled) return glIsEnabled(c); int b = en_bit(c); return b >= 0 ? (unsigned char)((g_enbits >> b) & 1) : 0; }
static __attribute__((used)) void w_glPixelStorei(unsigned p, int v) { ENTER("glPixelStorei", glPixelStorei); if (glPixelStorei) CALL(glPixelStorei, p, v); }
static __attribute__((used)) void w_glHint(unsigned t, unsigned m) { (void)t; (void)m; }
static __attribute__((used)) void w_gl_noop(void) {}
static __attribute__((used)) void SOFT w_glLightModelf(unsigned p, float v) { ENTER("glLightModelf", glLightModelf); if (glLightModelf) CALL(glLightModelf, p, v); else { float a[4] = { v, 0, 0, 0 }; CALL(glLightModelfv, p, a); } }
static __attribute__((used)) void w_glTexEnvi(unsigned t, unsigned p, int v) { ENTER("glTexEnvi", glTexEnvi); if (p == 0x2200) { g_tenv = (unsigned)v; g_u_env[g_act_unit] = (unsigned)v; } if (glTexEnvi) CALL(glTexEnvi, t, p, v); else CALL(glTexEnvf, t, p, (float)v); }
static __attribute__((used)) void w_glTexEnvfv(unsigned t, unsigned p, const float *v) { ENTER("glTexEnvfv", glTexEnvfv); if (glTexEnvfv) CALL(glTexEnvfv, t, p, v); }
static __attribute__((used)) void SOFT w_glDepthRangef(float n, float f) { ENTER("glDepthRangef", glDepthRangef); if (glDepthRangef) CALL(glDepthRangef, n, f); }
static __attribute__((used)) void SOFT w_glLineWidth(float w) { ENTER("glLineWidth", glLineWidth); if (glLineWidth) CALL(glLineWidth, w); }
static __attribute__((used)) void SOFT w_glPointSize(float z) { ENTER("glPointSize", glPointSize); if (glPointSize) CALL(glPointSize, z); }
static __attribute__((used)) void SOFT w_glPolygonOffset(float f, float u) { ENTER("glPolygonOffset", glPolygonOffset); if (glPolygonOffset) CALL(glPolygonOffset, f, u); }
static __attribute__((used)) void SOFT w_glColor4f(float r, float g, float b, float a) { ENTER("glColor4f", glColor4f);
    g_col[0] = (unsigned)(r * 255.f); g_col[1] = (unsigned)(g * 255.f); g_col[2] = (unsigned)(b * 255.f); g_col[3] = (unsigned)(a * 255.f); if (glColor4f) CALL(glColor4f, r, g, b, a); }
static __attribute__((used)) void SOFT w_glNormal3f(float x, float y, float z) { ENTER("glNormal3f", glNormal3f); if (glNormal3f) CALL(glNormal3f, x, y, z); }
static __attribute__((used)) void SOFT w_glMultiTexCoord4f(unsigned t, float s0, float t0, float r, float q) { (void)t; (void)s0; (void)t0; (void)r; (void)q; }
static __attribute__((used)) void w_glClipPlanef(unsigned p, const float *e) { ENTER("glClipPlanef", glClipPlanef); if (glClipPlanef) CALL(glClipPlanef, p, e); }
static __attribute__((used)) void w_glFlush(void) { ENTER("glFlush", glFlush); }
static __attribute__((used)) void w_glPushMatrix(void) { ENTER("glPushMatrix", glPushMatrix); int k = mstk(); float *cm = cur_mat(); if (k >= 0 && cm && g_msp[k] < 16) memcpy(g_mstack[k][g_msp[k]++], cm, 64); CALL(glPushMatrix); }
static __attribute__((used)) void w_glPopMatrix(void) { ENTER("glPopMatrix", glPopMatrix); int k = mstk(); float *cm = cur_mat(); if (k >= 0 && cm && g_msp[k] > 0) memcpy(cm, g_mstack[k][--g_msp[k]], 64); CALL(glPopMatrix); }
static __attribute__((used)) void SOFT w_glOrthof(float l, float r, float b, float t, float n, float f) { ENTER("glOrthof", glOrthof);
    float m[16] = { 2.f / (r - l), 0, 0, 0,  0, 2.f / (t - b), 0, 0,  0, 0, -2.f / (f - n), 0,  -(r + l) / (r - l), -(t + b) / (t - b), -(f + n) / (f - n), 1 };
    float *cm = cur_mat(); if (cm) { float o[16]; for (int c = 0; c < 4; c++) for (int rr = 0; rr < 4; rr++) o[c * 4 + rr] = cm[rr] * m[c * 4] + cm[4 + rr] * m[c * 4 + 1] + cm[8 + rr] * m[c * 4 + 2] + cm[12 + rr] * m[c * 4 + 3]; memcpy(cm, o, 64); }
    if (glOrthof) CALL(glOrthof, l, r, b, t, n, f); else CALL(glMultMatrixf, m); }
static __attribute__((used)) void w_glGetPointerv(unsigned pname, const void **out) {
    int k = pname == 0x808E ? 0 : pname == 0x808F ? 1 : pname == 0x8090 ? 2 : pname == 0x8092 ? 3 : -1;   /* vertex, normal, color, texcoord */
    if (out) *out = k >= 0 ? g_ap[k].ptr : NULL;
}
#define ALIGN_TRAMP(storage, tname, target) \
    storage __attribute__((naked, used)) void tname(void) { __asm__ volatile( \
        ".syntax unified\n" \
        "push {r4, r5, r6, lr}\n" \
        "mov r4, sp\n" \
        "sub r5, sp, #40\n" \
        "bic r5, r5, #7\n" \
        "mov sp, r5\n" \
        "add r5, r4, #16\n" \
        "ldmia r5, {r6, ip, lr}\n" \
        "stmia sp, {r6, ip, lr}\n" \
        "add r5, r5, #12\n" \
        "ldmia r5, {r6, ip, lr}\n" \
        "add r5, sp, #12\n" \
        "stmia r5, {r6, ip, lr}\n" \
        "bl " #target "\n" \
        "mov sp, r4\n" \
        "pop {r4, r5, r6, pc}\n"); }
ALIGN_TRAMP(static, t_glActiveTexture, w_glActiveTexture)
ALIGN_TRAMP(static, t_glAlphaFunc, w_glAlphaFunc)
ALIGN_TRAMP(static, t_glBindBuffer, w_glBindBuffer)
ALIGN_TRAMP(static, t_glBindFramebufferOES, w_glBindFramebufferOES)
ALIGN_TRAMP(static, t_glBindRenderbufferOES, w_glBindRenderbufferOES)
ALIGN_TRAMP(static, t_glBindTexture, w_glBindTexture)
ALIGN_TRAMP(static, t_glBlendEquationOES, w_glBlendEquationOES)
ALIGN_TRAMP(static, t_glBlendFunc, w_glBlendFunc)
ALIGN_TRAMP(static, t_glBufferData, w_glBufferData)
ALIGN_TRAMP(static, t_glBufferSubData, w_glBufferSubData)
ALIGN_TRAMP(static, t_glClear, w_glClear)
ALIGN_TRAMP(static, t_glClearColor, w_glClearColor)
ALIGN_TRAMP(static, t_glClearDepthf, w_glClearDepthf)
ALIGN_TRAMP(static, t_glClearStencil, w_glClearStencil)
ALIGN_TRAMP(static, t_glClientActiveTexture, w_glClientActiveTexture)
ALIGN_TRAMP(static, t_glColor4ub, w_glColor4ub)
ALIGN_TRAMP(static, t_glColorMask, w_glColorMask)
ALIGN_TRAMP(static, t_glColorPointer, w_glColorPointer)
ALIGN_TRAMP(static, t_glCompressedTexImage2D, w_glCompressedTexImage2D)
ALIGN_TRAMP(static, t_glCompressedTexSubImage2D, w_glCompressedTexSubImage2D)
ALIGN_TRAMP(static, t_glCopyTexImage2D, w_glCopyTexImage2D)
ALIGN_TRAMP(static, t_glCullFace, w_glCullFace)
ALIGN_TRAMP(static, t_glCurrentPaletteMatrixOES, w_glCurrentPaletteMatrixOES)
ALIGN_TRAMP(static, t_glDeleteBuffers, w_glDeleteBuffers)
ALIGN_TRAMP(static, t_glDeleteFramebuffersOES, w_glDeleteFramebuffersOES)
ALIGN_TRAMP(static, t_glDeleteRenderbuffersOES, w_glDeleteRenderbuffersOES)
ALIGN_TRAMP(static, t_glDeleteTextures, w_glDeleteTextures)
ALIGN_TRAMP(static, t_glDepthFunc, w_glDepthFunc)
ALIGN_TRAMP(static, t_glDepthMask, w_glDepthMask)
ALIGN_TRAMP(static, t_glDisable, w_glDisable)
ALIGN_TRAMP(static, t_glDisableClientState, w_glDisableClientState)
ALIGN_TRAMP(static, t_glDrawArrays, w_glDrawArrays)
ALIGN_TRAMP(static, t_glDrawElements, w_glDrawElements)
ALIGN_TRAMP(static, t_glEnable, w_glEnable)
ALIGN_TRAMP(static, t_glEnableClientState, w_glEnableClientState)
ALIGN_TRAMP(static, t_glFinish, w_glFinish)
ALIGN_TRAMP(static, t_glFogf, w_glFogf)
ALIGN_TRAMP(static, t_glFogfv, w_glFogfv)
ALIGN_TRAMP(static, t_glFramebufferRenderbufferOES, w_glFramebufferRenderbufferOES)
ALIGN_TRAMP(static, t_glFramebufferTexture2DOES, w_glFramebufferTexture2DOES)
ALIGN_TRAMP(static, t_glFrontFace, w_glFrontFace)
ALIGN_TRAMP(static, t_glGenBuffers, w_glGenBuffers)
ALIGN_TRAMP(static, t_glGenFramebuffersOES, w_glGenFramebuffersOES)
ALIGN_TRAMP(static, t_glGenRenderbuffersOES, w_glGenRenderbuffersOES)
ALIGN_TRAMP(static, t_glGenTextures, w_glGenTextures)
ALIGN_TRAMP(static, t_glGenerateMipmapOES, w_glGenerateMipmapOES)
ALIGN_TRAMP(static, t_glGetError, w_glGetError)
ALIGN_TRAMP(static, t_glGetFloatv, w_glGetFloatv)
ALIGN_TRAMP(static, t_glGetIntegerv, w_glGetIntegerv)
ALIGN_TRAMP(static, t_glGetRenderbufferParameterivOES, w_glGetRenderbufferParameterivOES)
ALIGN_TRAMP(static, t_glGetString, w_glGetString)
ALIGN_TRAMP(static, t_glLightModelfv, w_glLightModelfv)
ALIGN_TRAMP(static, t_glLightf, w_glLightf)
ALIGN_TRAMP(static, t_glLightfv, w_glLightfv)
ALIGN_TRAMP(static, t_glLoadIdentity, w_glLoadIdentity)
ALIGN_TRAMP(static, t_glLoadMatrixf, w_glLoadMatrixf)
ALIGN_TRAMP(static, t_glLoadPaletteFromModelViewMatrixOES, w_glLoadPaletteFromModelViewMatrixOES)
ALIGN_TRAMP(static, t_glMaterialf, w_glMaterialf)
ALIGN_TRAMP(static, t_glMaterialfv, w_glMaterialfv)
ALIGN_TRAMP(static, t_glMatrixIndexPointerOES, w_glMatrixIndexPointerOES)
ALIGN_TRAMP(static, t_glMatrixMode, w_glMatrixMode)
ALIGN_TRAMP(static, t_glMultMatrixf, w_glMultMatrixf)
ALIGN_TRAMP(static, t_glNormalPointer, w_glNormalPointer)
ALIGN_TRAMP(static, t_glReadPixels, w_glReadPixels)
ALIGN_TRAMP(static, t_glRenderbufferStorageOES, w_glRenderbufferStorageOES)
ALIGN_TRAMP(static, t_glScissor, w_glScissor)
ALIGN_TRAMP(static, t_glShadeModel, w_glShadeModel)
ALIGN_TRAMP(static, t_glCheckFramebufferStatusOES, w_glCheckFramebufferStatusOES)
ALIGN_TRAMP(static, t_glIsBuffer, w_glIsBuffer)
ALIGN_TRAMP(static, t_glIsTexture, w_glIsTexture)
ALIGN_TRAMP(static, t_glIsEnabled, w_glIsEnabled)
ALIGN_TRAMP(static, t_glPixelStorei, w_glPixelStorei)
ALIGN_TRAMP(static, t_glHint, w_glHint)
ALIGN_TRAMP(static, t_glLightModelf, w_glLightModelf)
ALIGN_TRAMP(static, t_glTexEnvi, w_glTexEnvi)
ALIGN_TRAMP(static, t_glTexEnvfv, w_glTexEnvfv)
ALIGN_TRAMP(static, t_glDepthRangef, w_glDepthRangef)
ALIGN_TRAMP(static, t_glLineWidth, w_glLineWidth)
ALIGN_TRAMP(static, t_glPointSize, w_glPointSize)
ALIGN_TRAMP(static, t_glPolygonOffset, w_glPolygonOffset)
ALIGN_TRAMP(static, t_glColor4f, w_glColor4f)
ALIGN_TRAMP(static, t_glNormal3f, w_glNormal3f)
ALIGN_TRAMP(static, t_glMultiTexCoord4f, w_glMultiTexCoord4f)
ALIGN_TRAMP(static, t_glClipPlanef, w_glClipPlanef)
ALIGN_TRAMP(static, t_glFlush, w_glFlush)
ALIGN_TRAMP(static, t_glPushMatrix, w_glPushMatrix)
ALIGN_TRAMP(static, t_glPopMatrix, w_glPopMatrix)
ALIGN_TRAMP(static, t_glOrthof, w_glOrthof)
ALIGN_TRAMP(static, t_gl_noop, w_gl_noop)
ALIGN_TRAMP(static, t_glGetPointerv, w_glGetPointerv)
ALIGN_TRAMP(static, t_glStencilFunc, w_glStencilFunc)
ALIGN_TRAMP(static, t_glStencilMask, w_glStencilMask)
ALIGN_TRAMP(static, t_glStencilOp, w_glStencilOp)
ALIGN_TRAMP(static, t_glTexCoordPointer, w_glTexCoordPointer)
ALIGN_TRAMP(static, t_glTexEnvf, w_glTexEnvf)
ALIGN_TRAMP(static, t_glTexImage2D, w_glTexImage2D)
ALIGN_TRAMP(static, t_glTexParameterf, w_glTexParameterf)
ALIGN_TRAMP(static, t_glTexParameteri, w_glTexParameteri)
ALIGN_TRAMP(static, t_glTexSubImage2D, w_glTexSubImage2D)
ALIGN_TRAMP(static, t_glVertexPointer, w_glVertexPointer)
ALIGN_TRAMP(static, t_glViewport, w_glViewport)
ALIGN_TRAMP(static, t_glWeightPointerOES, w_glWeightPointerOES)

ALIGN_TRAMP(, gl_present, gl_present_impl)
ALIGN_TRAMP(, gl_mark_drawable, gl_mark_drawable_impl)

typedef struct { const char *name; void *fn; } gshim_t;
static const gshim_t gtable[] = {
    {"_glActiveTexture", t_glActiveTexture}, {"_glAlphaFunc", t_glAlphaFunc}, {"_glBindBuffer", t_glBindBuffer},
    {"_glBindFramebufferOES", t_glBindFramebufferOES}, {"_glBindRenderbufferOES", t_glBindRenderbufferOES},
    {"_glBindTexture", t_glBindTexture}, {"_glBlendEquationOES", t_glBlendEquationOES}, {"_glBlendFunc", t_glBlendFunc},
    {"_glBufferData", t_glBufferData}, {"_glBufferSubData", t_glBufferSubData}, {"_glClear", t_glClear},
    {"_glClearColor", t_glClearColor}, {"_glClearDepthf", t_glClearDepthf}, {"_glClearStencil", t_glClearStencil},
    {"_glClientActiveTexture", t_glClientActiveTexture}, {"_glColor4ub", t_glColor4ub}, {"_glColorMask", t_glColorMask},
    {"_glColorPointer", t_glColorPointer}, {"_glCompressedTexImage2D", t_glCompressedTexImage2D},
    {"_glCompressedTexSubImage2D", t_glCompressedTexSubImage2D}, {"_glCopyTexImage2D", t_glCopyTexImage2D},
    {"_glCullFace", t_glCullFace}, {"_glCurrentPaletteMatrixOES", t_glCurrentPaletteMatrixOES},
    {"_glDeleteBuffers", t_glDeleteBuffers}, {"_glDeleteFramebuffersOES", t_glDeleteFramebuffersOES},
    {"_glDeleteRenderbuffersOES", t_glDeleteRenderbuffersOES}, {"_glDeleteTextures", t_glDeleteTextures},
    {"_glDepthFunc", t_glDepthFunc}, {"_glDepthMask", t_glDepthMask}, {"_glDisable", t_glDisable},
    {"_glDisableClientState", t_glDisableClientState}, {"_glDrawArrays", t_glDrawArrays}, {"_glDrawElements", t_glDrawElements},
    {"_glEnable", t_glEnable}, {"_glEnableClientState", t_glEnableClientState}, {"_glFinish", t_glFinish},
    {"_glFogf", t_glFogf}, {"_glFogfv", t_glFogfv}, {"_glFramebufferRenderbufferOES", t_glFramebufferRenderbufferOES},
    {"_glFramebufferTexture2DOES", t_glFramebufferTexture2DOES}, {"_glFrontFace", t_glFrontFace},
    {"_glGenBuffers", t_glGenBuffers}, {"_glGenFramebuffersOES", t_glGenFramebuffersOES},
    {"_glGenRenderbuffersOES", t_glGenRenderbuffersOES}, {"_glGenTextures", t_glGenTextures},
    {"_glGenerateMipmapOES", t_glGenerateMipmapOES}, {"_glGetError", t_glGetError}, {"_glGetFloatv", t_glGetFloatv},
    {"_glGetIntegerv", t_glGetIntegerv}, {"_glGetRenderbufferParameterivOES", t_glGetRenderbufferParameterivOES},
    {"_glGetString", t_glGetString}, {"_glLightModelfv", t_glLightModelfv}, {"_glLightf", t_glLightf}, {"_glLightfv", t_glLightfv},
    {"_glLoadIdentity", t_glLoadIdentity}, {"_glLoadMatrixf", t_glLoadMatrixf},
    {"_glLoadPaletteFromModelViewMatrixOES", t_glLoadPaletteFromModelViewMatrixOES}, {"_glMaterialf", t_glMaterialf},
    {"_glMaterialfv", t_glMaterialfv}, {"_glMatrixIndexPointerOES", t_glMatrixIndexPointerOES}, {"_glMatrixMode", t_glMatrixMode},
    {"_glMultMatrixf", t_glMultMatrixf}, {"_glNormalPointer", t_glNormalPointer}, {"_glReadPixels", t_glReadPixels},
    {"_glRenderbufferStorageOES", t_glRenderbufferStorageOES}, {"_glScissor", t_glScissor}, {"_glShadeModel", t_glShadeModel},
    {"_glStencilFunc", t_glStencilFunc}, {"_glStencilMask", t_glStencilMask}, {"_glStencilOp", t_glStencilOp},
    {"_glTexCoordPointer", t_glTexCoordPointer}, {"_glTexEnvf", t_glTexEnvf}, {"_glTexImage2D", t_glTexImage2D},
    {"_glTexParameterf", t_glTexParameterf}, {"_glTexParameteri", t_glTexParameteri}, {"_glTexSubImage2D", t_glTexSubImage2D},
    {"_glCheckFramebufferStatusOES", t_glCheckFramebufferStatusOES},
    {"_glIsBuffer", t_glIsBuffer},
    {"_glIsTexture", t_glIsTexture},
    {"_glIsEnabled", t_glIsEnabled},
    {"_glPixelStorei", t_glPixelStorei},
    {"_glHint", t_glHint},
    {"_glLightModelf", t_glLightModelf},
    {"_glTexEnvi", t_glTexEnvi},
    {"_glTexEnvfv", t_glTexEnvfv},
    {"_glDepthRangef", t_glDepthRangef},
    {"_glLineWidth", t_glLineWidth},
    {"_glPointSize", t_glPointSize},
    {"_glPolygonOffset", t_glPolygonOffset},
    {"_glColor4f", t_glColor4f},
    {"_glNormal3f", t_glNormal3f},
    {"_glMultiTexCoord4f", t_glMultiTexCoord4f},
    {"_glClipPlanef", t_glClipPlanef},
    {"_glFlush", t_glFlush},
    {"_glPushMatrix", t_glPushMatrix},
    {"_glPopMatrix", t_glPopMatrix},
    {"_glOrthof", t_glOrthof},
    {"_glGetPointerv", t_glGetPointerv}, {"_glSampleCoverage", t_gl_noop}, {"_glPointParameterf", t_gl_noop}, {"_glPointParameterfv", t_gl_noop}, {"_glDiscardFramebufferEXT", t_gl_noop},
    {"_glResolveMultisampleFramebufferAPPLE", t_gl_noop}, {"_glRenderbufferStorageMultisampleAPPLE", t_gl_noop},
    {"_glVertexPointer", t_glVertexPointer}, {"_glViewport", t_glViewport}, {"_glWeightPointerOES", t_glWeightPointerOES},
};
void *gl_shim_lookup(const char *name) {
    for (size_t i = 0; i < sizeof gtable / sizeof *gtable; i++) if (!strcmp(gtable[i].name, name)) return gtable[i].fn;
    return NULL;
}
