#include <stddef.h>

extern "C" size_t __malloc_object_size(const void*);

extern "C" size_t __dynamic_object_size(const void* ptr) {
  return __malloc_object_size(ptr);
}
