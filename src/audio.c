/* audio.c - AVAudioPlayer back-end for the Baroque loader.
 *
 * The game only uses AVAudioPlayer with CAF files:
 *   SE*.caf  : Apple IMA4 ADPCM, mono, 6-24 kHz (sound effects)
 *   BGM*.caf : AAC stereo 48 kHz in the IPA. AAC is not decoded here: tools/bgm.sh converts them once to IMA4 stereo
 *              (afconvert on the Mac) and uploads them over the originals.
 *
 * One mixer thread outputs 48 kHz stereo on a MAIN audio port. Every player owns one voice; IMA4 packets are
 * self-contained (each has its own predictor/step header), so voices decode packet by packet straight from the
 * compressed file kept in memory (small files are cached by path, large files are dropped again when idle).
 *
 * vita.cfg: volume=<0..2> master volume (default 1), nosound=1 disables audio. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include <psp2/audioout.h>

#define OUT_RATE   48000
#define GRAIN      1024              /* 21 ms per output buffer (256 was too short: stalls on the game threads made it crackle) */
#define MAX_SND    512
#define MAX_ACTIVE 48
#define BIG_FILE   (512 * 1024)       /* files bigger than this (music) are not kept resident while idle */

struct Voice;
typedef struct Snd {
    char path[200];
    uint8_t *data;        /* IMA4 packets (NULL = not loaded) */
    uint32_t npk, size;   /* packet count, file size */
    int ch, bpp;          /* channels, bytes per packet (34 * ch) */
    double rate;
    int big, users;       /* users = voices currently playing it */
    int bad;              /* 1 = unsupported format (logged once) */
    struct Voice *pool[8]; int npool;   /* voices created for this sound (reused once idle: the game makes a new player per play) */
} Snd;

typedef struct Voice {
    Snd *s;
    volatile int playing;
    int active;           /* in g_act */
    double pos, step;     /* source frames */
    int loops;            /* -1 = forever */
    float vol, pan;
    int dec_pk[2];
    int16_t dec[2][2][64];   /* two decoded packets (current and next) x channels */
} Voice;

static Snd g_snd[MAX_SND]; static int g_nsnd;
static Voice *g_act[MAX_ACTIVE]; static int g_nact;
static pthread_mutex_t g_mx = PTHREAD_MUTEX_INITIALIZER;      /* voice list (mixer) */
static pthread_mutex_t g_ldmx = PTHREAD_MUTEX_INITIALIZER;    /* sound table / file loading (never held by the mixer) */
extern int sceKernelChangeThreadPriority(int thid, int prio) __attribute__((weak));
extern int sceKernelChangeThreadCpuAffinityMask(int thid, int mask) __attribute__((weak));
static int g_core = 2;   /* vita.cfg audiocore=0..2 (-1 = let the scheduler decide) */
static unsigned g_reused;
static int g_port = -1, g_inited, g_nosound;
static float g_master = 1.0f;
static unsigned g_mixed_grains, g_played, g_failed;

static const int ima_step[89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143,
    157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552,
    1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487,
    12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767 };
static const int ima_idx[8] = { -1, -1, -1, -1, 2, 4, 6, 8 };

/* one 34-byte Apple IMA4 channel block -> 64 samples */
static void ima4_block(const uint8_t *b, int16_t *out) {
    int hdr = (b[0] << 8) | b[1];
    int pred = (int16_t)(hdr & 0xFF80), idx = hdr & 0x7F; if (idx > 88) idx = 88;
    for (int i = 0; i < 64; i++) {
        int nib = (b[2 + (i >> 1)] >> ((i & 1) ? 4 : 0)) & 0xF;
        int step = ima_step[idx], diff = step >> 3;
        if (nib & 4) diff += step; if (nib & 2) diff += step >> 1; if (nib & 1) diff += step >> 2;
        pred += (nib & 8) ? -diff : diff; if (pred > 32767) pred = 32767; else if (pred < -32768) pred = -32768;
        idx += ima_idx[nib & 7]; if (idx < 0) idx = 0; else if (idx > 88) idx = 88;
        out[i] = (int16_t)pred;
    }
}

static uint32_t be32(const uint8_t *p) { return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3]; }
static int64_t be64(const uint8_t *p) { return (int64_t)(((uint64_t)be32(p) << 32) | be32(p + 4)); }

/* load (or reload) a CAF file into s->data; returns 0 on success */
static int snd_load(Snd *s) {
    if (s->data) return 0;
    if (s->bad) return -1;
    FILE *f = fopen(s->path, "rb");
    if (!f) { fprintf(stderr, "[snd] cannot open %s\n", s->path); s->bad = 1; return -1; }
    fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
    uint8_t *d = malloc((size_t)n);
    if (!d || fread(d, 1, (size_t)n, f) != (size_t)n) { fclose(f); free(d); fprintf(stderr, "[snd] read failed %s (%ld bytes)\n", s->path, n); return -1; }
    fclose(f);
    if (n < 8 || memcmp(d, "caff", 4)) { free(d); s->bad = 1; fprintf(stderr, "[snd] not a CAF file: %s\n", s->path); return -1; }
    long o = 8; int have_desc = 0; uint8_t *audio = NULL; long alen = 0; char fmt[5] = "????";
    while (o + 12 <= n) {
        const uint8_t *t = d + o; int64_t sz = be64(t + 4); long body = o + 12;
        if (sz < 0 || body + sz > n) sz = n - body;
        if (!memcmp(t, "desc", 4) && sz >= 32) {
            union { uint64_t u; double v; } r; r.u = ((uint64_t)be32(d + body) << 32) | be32(d + body + 4);
            s->rate = r.v; memcpy(fmt, d + body + 8, 4); s->bpp = (int)be32(d + body + 16); s->ch = (int)be32(d + body + 24); have_desc = 1;
        } else if (!memcmp(t, "data", 4) && sz > 4) { audio = d + body + 4; alen = (long)sz - 4; }
        o = body + (long)sz;
    }
    if (!have_desc || !audio || memcmp(fmt, "ima4", 4) || (s->ch != 1 && s->ch != 2) || s->bpp != 34 * s->ch || s->rate < 1000) {
        fprintf(stderr, "[snd] unsupported %s: format '%s' ch=%d bpp=%d rate=%.0f%s\n", s->path, fmt, s->ch, s->bpp, s->rate,
                !memcmp(fmt, "aac ", 4) ? "  (AAC music: run tools/bgm.sh on the Mac to convert it to IMA4)" : "");
        free(d); s->bad = 1; return -1;
    }
    memmove(d, audio, (size_t)alen);                      /* keep only the packets */
    s->data = d; s->size = (uint32_t)n; s->npk = (uint32_t)(alen / s->bpp); s->big = n > BIG_FILE;
    return 0;
}

/* drop idle large files (music) to keep memory down; called with g_mx held */
static void snd_trim(Snd *keep) {
    for (int i = 0; i < g_nsnd; i++) { Snd *s = &g_snd[i]; if (s != keep && s->big && s->data && !s->users) { free(s->data); s->data = NULL; } }
}

static Snd *snd_get(const char *path) {
    for (int i = 0; i < g_nsnd; i++) if (!strcmp(g_snd[i].path, path)) return &g_snd[i];
    if (g_nsnd >= MAX_SND) return NULL;
    Snd *s = &g_snd[g_nsnd++]; memset(s, 0, sizeof *s); snprintf(s->path, sizeof s->path, "%s", path); return s;
}

static void voice_frame(Voice *v, uint32_t frame, int *l, int *r) {
    int pk = (int)(frame >> 6), slot = pk & 1;
    if (v->dec_pk[slot] != pk) {
        const uint8_t *p = v->s->data + (size_t)pk * (size_t)v->s->bpp;
        ima4_block(p, v->dec[slot][0]); if (v->s->ch == 2) ima4_block(p + 34, v->dec[slot][1]);
        v->dec_pk[slot] = pk;
    }
    *l = v->dec[slot][0][frame & 63]; *r = v->dec[slot][v->s->ch == 2 ? 1 : 0][frame & 63];
}
/* linear interpolation between frame and frame+1 (wrapping to 0 when looping) */
static void voice_sample(Voice *v, double pos, uint32_t nfr, int *l, int *r) {
    uint32_t f0 = (uint32_t)pos, f1 = f0 + 1; int frac = (int)((pos - (double)f0) * 256.0);
    if (f1 >= nfr) f1 = v->loops != 0 ? 0 : f0;
    int l0, r0, l1, r1; voice_frame(v, f0, &l0, &r0); voice_frame(v, f1, &l1, &r1);
    *l = l0 + (((l1 - l0) * frac) >> 8); *r = r0 + (((r1 - r0) * frac) >> 8);
}

static void act_remove(int i) { g_act[i]->s->users--; g_act[i]->active = 0; g_act[i] = g_act[--g_nact]; }

static void mix_grain(int16_t *out) {
    static int32_t acc[GRAIN * 2];
    memset(acc, 0, sizeof acc);
    pthread_mutex_lock(&g_mx);
    for (int vi = 0; vi < g_nact; vi++) {
        Voice *v = g_act[vi]; Snd *s = v->s;
        if (!v->playing || !s->data || !s->npk) { v->playing = 0; act_remove(vi--); continue; }
        uint32_t nfr = s->npk * 64;
        float gl = v->vol * g_master * (v->pan > 0.f ? 1.f - v->pan : 1.f), gr = v->vol * g_master * (v->pan < 0.f ? 1.f + v->pan : 1.f);
        int il = (int)(gl * 256.f), ir = (int)(gr * 256.f);
        for (int i = 0; i < GRAIN; i++) {
            if (v->pos >= (double)nfr) {
                if (v->loops != 0) { if (v->loops > 0) v->loops--; v->pos -= (double)nfr; }
                else { v->playing = 0; v->pos = 0; break; }
            }
            int l, r; voice_sample(v, v->pos, nfr, &l, &r);
            acc[i * 2] += (l * il) >> 8; acc[i * 2 + 1] += (r * ir) >> 8;
            v->pos += v->step;
        }
        if (!v->playing) act_remove(vi--);
    }
    pthread_mutex_unlock(&g_mx);
    for (int i = 0; i < GRAIN * 2; i++) { int x = acc[i]; out[i] = (int16_t)(x > 32767 ? 32767 : x < -32768 ? -32768 : x); }
}

static void *mixer_thread(void *arg) {
    (void)arg;
    static int16_t buf[2][GRAIN * 2]; int b = 0;
    if (sceKernelChangeThreadPriority) sceKernelChangeThreadPriority(0, 65);   /* above the game threads: audio must not starve */
    if (g_core >= 0 && g_core <= 2 && sceKernelChangeThreadCpuAffinityMask) {   /* pin to one user core (0x10000 << core) */
        int r = sceKernelChangeThreadCpuAffinityMask(0, 0x10000 << g_core);
        fprintf(stderr, "[snd] mixer thread pinned to core %d (%s 0x%x)\n", g_core, r < 0 ? "FAILED" : "ok", r); fflush(stderr); }
    for (;;) { mix_grain(buf[b]); sceAudioOutOutput(g_port, buf[b]); b ^= 1; g_mixed_grains++; }
    return NULL;
}

static void snd_init(void) {
    if (g_inited) return;
    g_inited = 1;
    FILE *f = fopen(GAME_DIR "vita.cfg", "r");
    if (f) { char line[128]; float v; while (fgets(line, sizeof line, f)) {
        if (sscanf(line, "volume=%f", &v) == 1 && v >= 0.f && v <= 2.f) g_master = v;
        else if (sscanf(line, "nosound=%f", &v) == 1) g_nosound = (int)v;
        else if (sscanf(line, "audiocore=%f", &v) == 1) g_core = (int)v; }
        fclose(f); }
    if (g_nosound) { fprintf(stderr, "[snd] disabled (nosound=1)\n"); return; }
    g_port = sceAudioOutOpenPort(SCE_AUDIO_OUT_PORT_TYPE_MAIN, GRAIN, OUT_RATE, SCE_AUDIO_OUT_MODE_STEREO);
    if (g_port < 0) { fprintf(stderr, "[snd] sceAudioOutOpenPort failed 0x%x\n", g_port); g_nosound = 1; return; }
    int vol[2] = { SCE_AUDIO_VOLUME_0DB, SCE_AUDIO_VOLUME_0DB };
    sceAudioOutSetVolume(g_port, SCE_AUDIO_VOLUME_FLAG_L_CH | SCE_AUDIO_VOLUME_FLAG_R_CH, vol);
    pthread_t th; pthread_attr_t at; pthread_attr_init(&at); pthread_attr_setstacksize(&at, 64 * 1024);
    if (pthread_create(&th, &at, mixer_thread, NULL)) { fprintf(stderr, "[snd] mixer thread failed\n"); g_nosound = 1; return; }
    fprintf(stderr, "[snd] audio ready: port %d, %d Hz stereo, master volume %.2f\n", g_port, OUT_RATE, (double)g_master); fflush(stderr);
}

/* ---------------- player API used by the AVAudioPlayer shim (objc_rt.c) ---------------- */
void *snd_player_new(const char *url) {
    snd_init();
    if (g_nosound) return NULL;                            /* nosound=1: behave as before audio existed (players are nil) */
    const char *p = url; if (!strncmp(p, "file://", 7)) p += 7; if (!strncmp(p, "localhost", 9)) p += 9;
    pthread_mutex_lock(&g_ldmx);                            /* file I/O happens here, without the mixer's lock */
    Snd *s = snd_get(p); int ok = s && snd_load(s) == 0;
    if (ok && s->big) { pthread_mutex_lock(&g_mx); snd_trim(s); pthread_mutex_unlock(&g_mx); }
    pthread_mutex_unlock(&g_ldmx);
    static int nlog; if (nlog < 12) { nlog++; fprintf(stderr, "[snd] player for %s -> %s\n", p, ok ? "ok" : "FAILED"); fflush(stderr); }
    if (!ok) { g_failed++; return NULL; }
    Voice *v = NULL;
    pthread_mutex_lock(&g_mx);
    for (int i = 0; i < s->npool; i++) if (!s->pool[i]->playing && !s->pool[i]->active) { v = s->pool[i]; g_reused++; break; }
    if (!v) { v = calloc(1, sizeof *v); if (v) { v->s = s; if (s->npool < 8) s->pool[s->npool++] = v; } }
    if (v) { v->vol = 1.f; v->pan = 0.f; v->loops = 0; v->pos = 0; v->dec_pk[0] = v->dec_pk[1] = -1; }
    pthread_mutex_unlock(&g_mx);
    return v;
}
int snd_play(void *h) {
    Voice *v = h; if (!v || g_nosound) return 0;
    pthread_mutex_lock(&g_ldmx);
    int ok = snd_load(v->s) == 0;                          /* reload a trimmed music file, outside the mixer's lock */
    pthread_mutex_lock(&g_mx);
    if (ok && !v->playing) {
        if (!v->active && g_nact < MAX_ACTIVE) { g_act[g_nact++] = v; v->active = 1; v->s->users++; }
        if (v->active) { v->step = v->s->rate / OUT_RATE; v->dec_pk[0] = v->dec_pk[1] = -1; v->playing = 1; g_played++; }
        if (v->s->big) snd_trim(v->s);
    }
    pthread_mutex_unlock(&g_mx);
    pthread_mutex_unlock(&g_ldmx);
    return ok;
}
void snd_stop(void *h) { Voice *v = h; if (v) v->playing = 0; }   /* AVAudioPlayer -stop keeps the position; the mixer drops the voice */
int snd_is_playing(void *h) { Voice *v = h; return v && v->playing; }
void snd_set_volume(void *h, float vol) { Voice *v = h; if (v) v->vol = vol < 0.f ? 0.f : vol > 1.f ? 1.f : vol; }
float snd_get_volume(void *h) { Voice *v = h; return v ? v->vol : 0.f; }
void snd_set_loops(void *h, int n) { Voice *v = h; if (v) v->loops = n < 0 ? -1 : n; }
void snd_set_pan(void *h, float p) { Voice *v = h; if (v) v->pan = p < -1.f ? -1.f : p > 1.f ? 1.f : p; }
void snd_stats(char *out, int n) { snprintf(out, (size_t)n, "[snd] voices=%d sounds=%d played=%u reused=%u failed=%u buffers=%u", g_nact, g_nsnd, g_played, g_reused, g_failed, g_mixed_grains); }
