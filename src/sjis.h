#include <stddef.h>
size_t sjis_from_utf8(const char *u8, unsigned char *out);
size_t sjis_to_utf8(const unsigned char *in, size_t n, char *out);
