#ifndef LIBC_BIONIC_OMALLOC_H_
#define LIBC_BIONIC_OMALLOC_H_

#include <malloc.h>
#include <sys/cdefs.h>
#include <stddef.h>

__BEGIN_DECLS
void *o_malloc(size_t size);
void o_free(void *ptr);
void *o_realloc(void *ptr, size_t size);
void *o_calloc(size_t nmemb, size_t size);
int o_posix_memalign(void **memptr, size_t alignment, size_t size);
void *o_memalign(size_t boundary, size_t size);
void *o_valloc(size_t size);
void *o_pvalloc(size_t size);
size_t o_malloc_usable_size(const void *p);
struct mallinfo o_mallinfo(void);
int o_mallopt(int, int);
__END_DECLS

#endif  // LIBC_BIONIC_OMALLOC_H_
