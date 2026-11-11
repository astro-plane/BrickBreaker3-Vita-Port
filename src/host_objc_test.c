/* Host test: guest ObjC metadata parsing + dispatch to native stand-ins (no ARM code is executed). */
#define _GNU_SOURCE
#include "macho.h"
#include "objc_rt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include "binds_gen.h"
extern void *rt_lookup(uint32_t cls, const char *sel);
extern const char *sel_registerName(const char *);
extern void *libc_data_lookup(const char *);
uint8_t *platform_alloc_image(uint32_t size) { return mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_32BIT, -1, 0); }
void platform_flush_icache(void *p, uint32_t s) {}
uint32_t platform_resolve(int idx) {                       /* class/metaclass data symbols become real native classes */
    if (import_is_data[idx]) return (uint32_t)(uintptr_t)libc_data_lookup(import_names[idx]);
    return 0xF0000000u + idx;
}
int main(int argc, char **argv) {
    FILE *fp = fopen(argv[1], "rb"); fseek(fp, 0, SEEK_END); size_t len = ftell(fp); rewind(fp);
    uint8_t *f = malloc(len); fread(f, 1, len, fp);
    macho_t m; macho_parse(&m, f, len); macho_map(&m, f); macho_rebase(&m, f); macho_bind(&m);
    objc_rt_init(&m);
    printf("classes registered: %d (6 guest expected, rest native)\n", objc_class_count());
    objc_dump_classes();
    /* a guest selref must now be a uniqued selector */
    uint32_t *ref = macho_ptr(&m, m.selrefs_addr);
    printf("selref[0] -> '%s' (same pointer as sel_registerName: %s)\n", (const char *)(uintptr_t)ref[0], (const char *)(uintptr_t)ref[0] == sel_registerName((const char *)(uintptr_t)ref[0]) ? "yes" : "NO");
    return 0;
}
