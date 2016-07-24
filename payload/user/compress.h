#ifndef COMPRESS_H
#define COMPRESS_H

#include <sys/types.h>

size_t decompress(void *s_start, void *d_start, size_t s_len, size_t d_len);

#endif // COMPRESS_H
