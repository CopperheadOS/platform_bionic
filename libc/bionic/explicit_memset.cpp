#undef _FORTIFY_SOURCE
#include <string.h>

void *explicit_memset(void *s, int c, size_t n) {
    void *ptr = memset(s, c, n);
    __asm__ __volatile__("" : : "r"(ptr) : "memory");
    return ptr;
}
