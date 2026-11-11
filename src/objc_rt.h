#ifndef OBJC_RT_H
#define OBJC_RT_H
#include "macho.h"
void  objc_rt_init(macho_t *m);                 /* load game classes, uniquify selectors (call before commit) */
void *objc_native_class_symbol(const char *sym);/* _OBJC_CLASS_$_X / _OBJC_METACLASS_$_X / ___CFConstantStringClassReference */
void *objc_import_lookup(const char *name);     /* objc_msgSend & friends, UIApplicationMain, NSLog ... */
void *objc_g_alloc(unsigned size);              /* zeroed memory addressable by 32-bit guest pointers */
int   objc_class_count(void);
void  objc_dump_classes(void);
#endif
