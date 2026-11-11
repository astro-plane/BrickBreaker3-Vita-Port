/* Host-side validation of the loader logic (no ARM code is executed). */
#define _GNU_SOURCE
#include "macho.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include "binds_gen.h"

uint8_t *platform_alloc_image(uint32_t size) {
    void *p = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_32BIT, -1, 0);
    return p == MAP_FAILED ? NULL : p;
}
void platform_flush_icache(void *p, uint32_t s) { (void)p; (void)s; }
uint32_t platform_resolve(int idx) { return 0xF0000000u + idx; }   /* fake handle, recognizable */

int main(int argc, char **argv) {
    if (argc < 2) return 2;
    FILE *fp = fopen(argv[1], "rb"); if (!fp) { perror("open"); return 1; }
    fseek(fp, 0, SEEK_END); size_t len = ftell(fp); rewind(fp);
    uint8_t *f = malloc(len); if (fread(f, 1, len, fp) != len) return 1; fclose(fp);

    macho_t m;
    int r = macho_parse(&m, f, len); if (r) return 1;
    printf("segments: %d  vm_low=0x%x vm_size=0x%x  entry_pc=0x%x  inits=%u@0x%x  cryptid=%d\n",
           m.nseg, m.vm_low, m.vm_size, m.entry_pc, m.mod_init_count, m.mod_init_addr, m.cryptid);
    if (macho_map(&m, f)) return 1;
    uint32_t base = (uint32_t)(uintptr_t)m.base;
    macho_rebase(&m, f);
    printf("rebased slots: %u\n", m.n_rebased);
    macho_bind(&m);
    printf("bound slots:   %u (of %d sites, %d imports)\n", m.n_bound, N_BIND_SITES, N_IMPORTS);

    /* sanity: every mod_init pointer must land inside the image (after slide) and be code */
    int bad = 0;
    for (uint32_t i = 0; i < m.mod_init_count; i++) {
        uint32_t v = macho_ptr(&m, m.mod_init_addr)[i];
        uint32_t off = (v & ~1u) - base;
        if (off >= m.seg[1].vmsize) bad++;
    }
    printf("mod_init pointers outside __TEXT: %d / %u\n", bad, m.mod_init_count);
    /* sanity: all non-bound pointer-looking slots in __DATA that were rebased point inside image */
    uint32_t outside = 0, thumb = 0;
    for (uint32_t a = m.seg[2].vmaddr; a < m.seg[2].vmaddr + m.seg[2].filesize; a += 4) {
        uint32_t v = *macho_ptr(&m, a);
        if (v >= base && v < base + m.vm_size) { if (v & 1) thumb++; } else if (v >= 0xF0000000u) { } else if (v > 0x1000 && v < 0xFFFF0000u && (v & 0xFFF00000u) == (base & 0xFFF00000u)) outside++;
    }
    printf("__DATA words pointing into image: odd(thumb) targets=%u, suspicious near-image-but-outside=%u\n", thumb, outside);
    printf("main would be at host 0x%x\n", base + (0x2208 - m.vm_low));
    return 0;
}
