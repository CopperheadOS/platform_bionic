#include <stddef.h>
#include "private/bionic_globals.h"
#include "pthread_internal.h"

extern "C" size_t __malloc_object_size(const void*);
int __pthread_attr_getstack_main_thread(void** stack_base, size_t* stack_size);

extern "C" size_t __dynamic_object_size(const void* ptr) {
  if (__predict_false(!__libc_globals->enable_object_size_checks || __get_thread()->in_malloc)) {
    return __BIONIC_FORTIFY_UNKNOWN_SIZE;
  }

  pthread_internal_t* thread = __get_thread();

  void* stack_base = nullptr;
  void* stack_top = nullptr;
  void* stack_frame = __builtin_frame_address(0);

  if (thread->tid == getpid()) {
    if (__libc_globals->main_thread_stack_top) {
      stack_base = stack_frame;
      stack_top = __libc_globals->main_thread_stack_top;
    }
  } else {
    stack_base = thread->attr.stack_base;
    stack_top = static_cast<char*>(stack_base) + thread->attr.stack_size;
  }

  if (ptr > stack_base && ptr < stack_top) {
    if (__predict_false(ptr < stack_frame)) {
      async_safe_fatal("%p is an invalid object address (in unused stack space %p-%p)", ptr, stack_base,
                       stack_frame);
    }
    return static_cast<char*>(stack_top) - static_cast<char*>(const_cast<void*>(ptr));
  }

  return __malloc_object_size(ptr);
}

void __libc_init_dynamic_object_size(libc_globals* globals) {
  globals->enable_object_size_checks = true;

  void* current_base;
  size_t current_size;
  if (access("/proc/self/stat", R_OK) == -1) {
    return;
  }
  if (__pthread_attr_getstack_main_thread(&current_base, &current_size)) {
    return;
  }
  globals->main_thread_stack_top = static_cast<char*>(current_base) + current_size;
}
