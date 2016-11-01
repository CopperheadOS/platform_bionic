#include <link.h>
#include <elf.h>
#include <stddef.h>
#include "private/bionic_globals.h"
#include "private/bionic_page.h"
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

  if (ptr > __libc_globals->executable_start && ptr < __libc_globals->executable_end) {
    return static_cast<char*>(__libc_globals->executable_end) - static_cast<char*>(const_cast<void*>(ptr));
  }

  return __malloc_object_size(ptr);
}

static size_t phdr_table_get_load_size(const ElfW(Phdr)* phdr_table, size_t phdr_count,
                                       ElfW(Addr)* out_min_vaddr,
                                       ElfW(Addr)* out_max_vaddr) {
  ElfW(Addr) min_vaddr = UINTPTR_MAX;
  ElfW(Addr) max_vaddr = 0;

  bool found_pt_load = false;
  for (size_t i = 0; i < phdr_count; ++i) {
    const ElfW(Phdr)* phdr = &phdr_table[i];

    if (phdr->p_type != PT_LOAD) {
      continue;
    }
    found_pt_load = true;

    if (phdr->p_vaddr < min_vaddr) {
      min_vaddr = phdr->p_vaddr;
    }

    if (phdr->p_vaddr + phdr->p_memsz > max_vaddr) {
      max_vaddr = phdr->p_vaddr + phdr->p_memsz;
    }
  }
  if (!found_pt_load) {
    min_vaddr = 0;
  }

  min_vaddr = PAGE_START(min_vaddr);
  max_vaddr = PAGE_END(max_vaddr);

  if (out_min_vaddr != nullptr) {
    *out_min_vaddr = min_vaddr;
  }
  if (out_max_vaddr != nullptr) {
    *out_max_vaddr = max_vaddr;
  }
  return max_vaddr - min_vaddr;
}

static int callback(struct dl_phdr_info *info, size_t, void* data) {
  void* addr = reinterpret_cast<void*>(info->dlpi_addr);
  if (addr == nullptr) {
    return 0;
  }
  size_t size = phdr_table_get_load_size(info->dlpi_phdr, info->dlpi_phnum, nullptr, nullptr);
  libc_globals* globals = static_cast<libc_globals*>(data);
  globals->executable_start = addr;
  globals->executable_end = static_cast<char*>(addr) + size;
  return 1;
}

void __libc_init_dynamic_object_size(libc_globals* globals) {
  globals->enable_object_size_checks = true;

  dl_iterate_phdr(callback, globals);

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
