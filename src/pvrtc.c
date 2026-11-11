/* Software PVRTC1 decoder (RGB/RGBA, 4bpp and 2bpp) -> RGBA8888. Used as a diagnostic/fallback when vita.cfg has swpvrtc=1.
 * variant: bit0 = swap the X/Y roles in the Morton block order (in case the default order is wrong). Returns 0 on success. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct { int r, g, b, a; } PC;

static int e5(int v) { return (v << 3) | (v >> 2); }
static int e4(int v) { return v * 17; }
static int e3(int v) { return (v << 5) | (v << 2) | (v >> 1); }

static PC col_a(uint32_t w) {
    PC c;
    if (w & 0x8000u) { int r = (w >> 10) & 31, g = (w >> 5) & 31, b = (w >> 1) & 15; b = (b << 1) | (b >> 3); c.r = e5(r); c.g = e5(g); c.b = e5(b); c.a = 255; }
    else { int a = (w >> 12) & 7, r = (w >> 8) & 15, g = (w >> 4) & 15, b = (w >> 1) & 7; c.a = e3(a); c.r = e4(r); c.g = e4(g); c.b = e3(b); }
    return c;
}
static PC col_b(uint32_t w) {
    PC c;
    if (w & 0x80000000u) { int r = (w >> 26) & 31, g = (w >> 21) & 31, b = (w >> 16) & 31; c.r = e5(r); c.g = e5(g); c.b = e5(b); c.a = 255; }
    else { int a = (w >> 28) & 7, r = (w >> 24) & 15, g = (w >> 20) & 15, b = (w >> 16) & 15; c.a = e3(a); c.r = e4(r); c.g = e4(g); c.b = e4(b); }
    return c;
}

static uint32_t twiddle(int bw, int bh, int bx, int by, int swap) {
    int mind = bw < bh ? bw : bh;
    int a = swap ? bx : by, b = swap ? by : bx;     /* 'a' occupies the even bits */
    uint32_t t = 0, src = 1, dst = 1; int sc = 0;
    while ((int)src < mind) { if (a & src) t |= dst; if (b & src) t |= dst << 1; src <<= 1; dst <<= 2; sc++; }
    int maxv = (bh < bw) ? bx : by;
    maxv >>= sc; t |= (uint32_t)maxv << (2 * sc);
    return t;
}

int pvrtc_decode(const uint8_t *src, int w, int h, int bpp2, int has_alpha, int variant, uint8_t *dst) {
    int bwpix = bpp2 ? 8 : 4;
    int bw = w / bwpix, bh = h / 4;
    if (bw < 2 || bh < 2 || (bw & (bw - 1)) || (bh & (bh - 1))) return -1;
    const uint32_t *blk = (const uint32_t *)src;
    PC *A = malloc(sizeof(PC) * (size_t)bw * bh), *B = malloc(sizeof(PC) * (size_t)bw * bh);
    uint32_t *M = malloc(sizeof(uint32_t) * (size_t)bw * bh), *Mode = malloc(sizeof(uint32_t) * (size_t)bw * bh);
    if (!A || !B || !M || !Mode) { free(A); free(B); free(M); free(Mode); return -2; }
    for (int by = 0; by < bh; by++) for (int bx = 0; bx < bw; bx++) {
        uint32_t idx = twiddle(bw, bh, bx, by, variant & 1);
        uint32_t m = blk[idx * 2], c = blk[idx * 2 + 1];
        A[by * bw + bx] = col_a(c); B[by * bw + bx] = col_b(c); M[by * bw + bx] = m; Mode[by * bw + bx] = c & 1;
    }
    for (int y = 0; y < h; y++) {
        int uy = y - 2, by0 = uy >> 2, fy = uy & 3;
        int by1 = (by0 + 1) & (bh - 1); by0 &= (bh - 1);
        for (int x = 0; x < w; x++) {
            int ux, bx0, fx, bx1, fxden;
            if (bpp2) { ux = x - 4; bx0 = ux >> 3; fx = ux & 7; fxden = 8; } else { ux = x - 2; bx0 = ux >> 2; fx = ux & 3; fxden = 4; }
            bx1 = (bx0 + 1) & (bw - 1); bx0 &= (bw - 1);
            int w00 = (fxden - fx) * (4 - fy), w10 = fx * (4 - fy), w01 = (fxden - fx) * fy, w11 = fx * fy, den = fxden * 4;
            const PC *a00 = &A[by0 * bw + bx0], *a10 = &A[by0 * bw + bx1], *a01 = &A[by1 * bw + bx0], *a11 = &A[by1 * bw + bx1];
            const PC *b00 = &B[by0 * bw + bx0], *b10 = &B[by0 * bw + bx1], *b01 = &B[by1 * bw + bx0], *b11 = &B[by1 * bw + bx1];
            int ar = (a00->r * w00 + a10->r * w10 + a01->r * w01 + a11->r * w11) / den;
            int ag = (a00->g * w00 + a10->g * w10 + a01->g * w01 + a11->g * w11) / den;
            int ab = (a00->b * w00 + a10->b * w10 + a01->b * w01 + a11->b * w11) / den;
            int aa = (a00->a * w00 + a10->a * w10 + a01->a * w01 + a11->a * w11) / den;
            int br = (b00->r * w00 + b10->r * w10 + b01->r * w01 + b11->r * w11) / den;
            int bg = (b00->g * w00 + b10->g * w10 + b01->g * w01 + b11->g * w11) / den;
            int bb = (b00->b * w00 + b10->b * w10 + b01->b * w01 + b11->b * w11) / den;
            int ba = (b00->a * w00 + b10->a * w10 + b01->a * w01 + b11->a * w11) / den;
            int bi = (y >> 2) * bw + (x >> bpp2 * 1 / 1 >> (bpp2 ? 3 : 2));   /* block containing this pixel */
            bi = (y >> 2) * bw + (bpp2 ? (x >> 3) : (x >> 2));
            int wt;
            if (bpp2) { int bit = (M[bi] >> (((y & 3) * 8) + (x & 7))) & 1; wt = bit ? 8 : 0; }
            else { int m = (M[bi] >> (((y & 3) * 4 + (x & 3)) * 2)) & 3; wt = Mode[bi] ? (m == 0 ? 0 : (m == 3 ? 8 : 4)) : (m == 0 ? 0 : m == 1 ? 3 : m == 2 ? 5 : 8); }
            uint8_t *o = dst + ((size_t)y * w + x) * 4;
            o[0] = (uint8_t)((ar * (8 - wt) + br * wt) / 8);
            o[1] = (uint8_t)((ag * (8 - wt) + bg * wt) / 8);
            o[2] = (uint8_t)((ab * (8 - wt) + bb * wt) / 8);
            o[3] = has_alpha ? (uint8_t)((aa * (8 - wt) + ba * wt) / 8) : 255;
        }
    }
    free(A); free(B); free(M); free(Mode);
    return 0;
}
