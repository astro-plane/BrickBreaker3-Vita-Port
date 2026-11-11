#include "macho.h"
#include <string.h>
#include <stdio.h>
#include "binds_gen.h"

#define MH_MAGIC 0xfeedfaceu
#define LC_SEGMENT 0x1
#define LC_UNIXTHREAD 0x5
#define LC_DYLD_INFO_ONLY 0x80000022u
#define LC_ENCRYPTION_INFO 0x21

typedef struct { uint32_t magic; int32_t cputype, cpusubtype; uint32_t filetype, ncmds, sizeofcmds, flags; } mh_t;
typedef struct { uint32_t cmd, cmdsize; } lc_t;
typedef struct { uint32_t cmd, cmdsize; char segname[16]; uint32_t vmaddr, vmsize, fileoff, filesize, maxprot, initprot, nsects, flags; } seg_t;
typedef struct { char sectname[16], segname[16]; uint32_t addr, size, offset, align, reloff, nreloc, flags, r1, r2; } sect_t;
typedef struct { uint32_t cmd, cmdsize, rebase_off, rebase_size, bind_off, bind_size, weak_off, weak_size, lazy_off, lazy_size, export_off, export_size; } dyld_t;

int macho_parse(macho_t *m, const uint8_t *f, size_t len) {
    memset(m, 0, sizeof *m);
    const mh_t *h = (const mh_t *)f;
    if (len < sizeof *h || h->magic != MH_MAGIC || h->cputype != 12) { fprintf(stderr, "not a 32-bit ARM Mach-O\n"); return -1; }
    const uint8_t *p = f + sizeof *h;
    m->vm_low = 0xffffffffu; m->file_len = (uint32_t)len;
    uint32_t hi = 0;
    for (uint32_t i = 0; i < h->ncmds; i++) {
        const lc_t *c = (const lc_t *)p;
        if (c->cmd == LC_SEGMENT) {
            const seg_t *s = (const seg_t *)p;
            if (m->nseg < 8) {
                memcpy(m->seg[m->nseg].name, s->segname, 16);
                m->seg[m->nseg].vmaddr = s->vmaddr; m->seg[m->nseg].vmsize = s->vmsize;
                m->seg[m->nseg].fileoff = s->fileoff; m->seg[m->nseg].filesize = s->filesize;
                m->nseg++;
            }
            if (strcmp(s->segname, "__PAGEZERO")) {
                if (s->vmaddr < m->vm_low) m->vm_low = s->vmaddr;
                if (s->vmaddr + s->vmsize > hi) hi = s->vmaddr + s->vmsize;
            }
            const sect_t *sc = (const sect_t *)(p + sizeof *s);
            for (uint32_t k = 0; k < s->nsects; k++)
                if (!strncmp(sc[k].sectname, "__mod_init_func", 16)) { m->mod_init_addr = sc[k].addr; m->mod_init_count = sc[k].size / 4; }
                else if (!strncmp(sc[k].sectname, "__objc_classlist", 16)) { m->classlist_addr = sc[k].addr; m->classlist_count = sc[k].size / 4; }
                else if (!strncmp(sc[k].sectname, "__objc_selrefs", 16)) { m->selrefs_addr = sc[k].addr; m->selrefs_count = sc[k].size / 4; }
        } else if (c->cmd == LC_UNIXTHREAD) {
            const uint32_t *t = (const uint32_t *)(p + 16);   /* flavor,count, then r0..r12,sp,lr,pc,cpsr */
            m->entry_pc = t[15];
        } else if (c->cmd == LC_DYLD_INFO_ONLY) {
            const dyld_t *d = (const dyld_t *)p; m->rebase_off = d->rebase_off; m->rebase_size = d->rebase_size;
        } else if (c->cmd == LC_ENCRYPTION_INFO) {
            m->cryptid = ((const uint32_t *)p)[4];
        }
        p += c->cmdsize;
    }
    m->vm_size = (hi - m->vm_low + 0xfff) & ~0xfffu;
    if (m->cryptid) { fprintf(stderr, "binary is still FairPlay-encrypted (cryptid=%d)\n", m->cryptid); return -2; }
    return 0;
}

int macho_map(macho_t *m, const uint8_t *f) {
    m->base = platform_alloc_image(m->vm_size);
    if (!m->base) return -1;
    for (int i = 0; i < m->nseg; i++) {
        if (!strcmp(m->seg[i].name, "__PAGEZERO") || !m->seg[i].filesize) continue;
        memcpy(m->base + (m->seg[i].vmaddr - m->vm_low), f + m->seg[i].fileoff, m->seg[i].filesize);
    }
    return 0;
}

static uint32_t uleb(const uint8_t **p) { uint32_t r = 0; int s = 0; uint8_t b; do { b = *(*p)++; r |= (uint32_t)(b & 0x7f) << s; s += 7; } while (b & 0x80); return r; }

#include "reloc_gen.h"
/* Non-PIE binaries (built for iOS < 4.3) carry no rebase info: they expect to sit at their link address. The loader
 * then applies a relocation list synthesized offline (tools/nonpie_reloc.py -> reloc_gen.h) for that exact binary. */
static int nonpie_rebase(macho_t *m, uint32_t slide) {
    if (m->file_len != NONPIE_RELOC_FILESIZE) {
        fprintf(stderr, "non-PIE binary (no rebase info) and reloc_gen.h was made for a different file (%u bytes, this one %u)\n", NONPIE_RELOC_FILESIZE, m->file_len);
        return -1;
    }
    for (unsigned i = 0; i < NONPIE_RELOC_COUNT; i++) { *macho_ptr(m, nonpie_reloc[i]) += slide; m->n_rebased++; }
    fprintf(stderr, "non-PIE binary: applied %u synthesized relocations (slide %08x)\n", NONPIE_RELOC_COUNT, slide);
    return 0;
}

int macho_rebase(macho_t *m, const uint8_t *f) {
    const uint8_t *p = f + m->rebase_off, *end = p + m->rebase_size;
    uint32_t slide = (m->final_base ? m->final_base : (uint32_t)(uintptr_t)m->base) - m->vm_low;
    uint32_t addr = 0; m->n_rebased = 0;
    if (!m->rebase_size) return slide ? nonpie_rebase(m, slide) : 0;
    #define REBASE1() do { uint32_t *q = macho_ptr(m, addr); *q += slide; m->n_rebased++; } while (0)
    while (p < end) {
        uint8_t op = *p & 0xF0, imm = *p & 0x0F; p++;
        switch (op) {
        case 0x00: return 0;
        case 0x10: break;                                           /* SET_TYPE_IMM (pointer only) */
        case 0x20: addr = m->seg[imm].vmaddr + uleb(&p); break;
        case 0x30: addr += uleb(&p); break;
        case 0x40: addr += imm * 4; break;
        case 0x50: for (int i = 0; i < imm; i++) { REBASE1(); addr += 4; } break;
        case 0x60: { uint32_t n = uleb(&p); while (n--) { REBASE1(); addr += 4; } break; }
        case 0x70: REBASE1(); addr += uleb(&p) + 4; break;
        case 0x80: { uint32_t n = uleb(&p), skip = uleb(&p); while (n--) { REBASE1(); addr += 4 + skip; } break; }
        default: fprintf(stderr, "bad rebase opcode %02x\n", op); return -1;
        }
    }
    return 0;
}

int macho_bind(macho_t *m) {
    m->n_bound = 0;
    for (unsigned i = 0; i < N_BIND_SITES; i++) {
        *macho_ptr(m, bind_sites[i].addr) = platform_resolve(bind_sites[i].idx);
        m->n_bound++;
    }
    return 0;
}
