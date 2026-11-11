#ifndef MACHO_H
#define MACHO_H
#include <stdint.h>
#include <stddef.h>

/* Minimal 32-bit Mach-O (armv7) loader. All guest addresses are uint32_t. */
typedef struct {
    uint8_t  *base;        /* staging buffer corresponding to guest vmaddr `vm_low` */
    uint32_t  final_base;  /* address vm_low will live at when committed (0 = same as base) */
    uint32_t  vm_low;      /* lowest mapped vmaddr (0x1000 for __TEXT) */
    uint32_t  vm_size;     /* span to allocate */
    uint32_t  entry_pc;    /* LC_UNIXTHREAD pc (crt `start`) */
    uint32_t  mod_init_addr, mod_init_count;
    uint32_t  classlist_addr, classlist_count, selrefs_addr, selrefs_count;
    uint32_t  rebase_off, rebase_size;
    struct { char name[17]; uint32_t vmaddr, vmsize, fileoff, filesize; } seg[8];
    int       nseg;
    int       cryptid;
    uint32_t  file_len;
    uint32_t  n_rebased, n_bound;
} macho_t;

/* platform hooks */
uint8_t *platform_alloc_image(uint32_t size);          /* zeroed, RWX */
void     platform_flush_icache(void *p, uint32_t size);
uint32_t platform_resolve(int import_idx);             /* returns guest-visible 32-bit value for the slot */

int macho_parse(macho_t *m, const uint8_t *file, size_t len);
int macho_map(macho_t *m, const uint8_t *file);        /* alloc + copy segments */
int macho_rebase(macho_t *m, const uint8_t *file);     /* interpret dyld rebase opcodes */
int macho_bind(macho_t *m);                            /* write resolved imports via generated bind_sites */
static inline uint32_t *macho_ptr(macho_t *m, uint32_t vmaddr) { return (uint32_t *)(m->base + (vmaddr - m->vm_low)); }
#endif
