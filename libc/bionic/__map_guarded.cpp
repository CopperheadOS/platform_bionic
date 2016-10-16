#include <sys/mman.h>
#include <sys/user.h>

void* __map_guarded(size_t usable_size) {
  size_t real_size = usable_size + PAGE_SIZE * 2;
  if (real_size < usable_size) {
    return MAP_FAILED;
  }
  void* real = mmap(NULL, real_size, PROT_NONE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  if (real == MAP_FAILED) {
    return real;
  }
  void* usable = reinterpret_cast<char*>(real) + PAGE_SIZE;
  if (mprotect(usable, usable_size, PROT_READ|PROT_WRITE) == -1) {
    munmap(real, real_size);
    return MAP_FAILED;
  }
  return usable;
}

int __unmap_guarded(void* usable, size_t usable_size) {
  void *real = reinterpret_cast<char*>(usable) - PAGE_SIZE;
  return munmap(real, usable_size + PAGE_SIZE * 2);
}
