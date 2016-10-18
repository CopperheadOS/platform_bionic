#include <stddef.h>
#include "private/bionic_globals.h"
#include "pthread_internal.h"

extern "C" size_t __malloc_object_size(const void*);

extern "C" size_t __dynamic_object_size(const void* ptr) {
  if (__predict_false(!__libc_globals->enable_object_size_checks || __get_thread()->in_malloc)) {
    return __BIONIC_FORTIFY_UNKNOWN_SIZE;
  }
  return __malloc_object_size(ptr);
}

void __libc_init_dynamic_object_size(libc_globals* globals) {
  globals->enable_object_size_checks = true;
}
