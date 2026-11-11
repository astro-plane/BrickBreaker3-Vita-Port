/* bqlog.h - force-included into every source file (CMakeLists: -include).
 * stderr (loader.txt) is fully buffered and written by a low-priority thread on core 0 every 0.25 s: any write to the
 * memory card blocks the writing thread 20-80 ms, which made every log line a potential hitch on the game thread.
 * fflush(stderr) is therefore a no-op unless vita.cfg has synclog=1 (immediate writes, for chasing crashes). */
#ifndef BQLOG_H
#define BQLOG_H
#include <stdio.h>
#include "game.h"
int bq_fflush(FILE *f);
#define fflush(f) bq_fflush(f)
#endif
