/*-
 * Copyright (c) 2011 David Chisnall
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD$
 */

#include <stdlib.h>
#include <pthread.h>
#include <sys/mman.h>
#include <unistd.h>

#include "private/bionic_prctl.h"

/**
 * Linked list of quick exit handlers.  This is simpler than the atexit()
 * version, because it is not required to support C++ destructors or
 * DSO-specific cleanups.
 */
struct quick_exit_handler {
	struct quick_exit_handler *next;
	void (*cleanup)(void);
};

static struct quick_exit_handler *pool_page;
static struct quick_exit_handler *pool;
static size_t pool_size;

/**
 * Lock protecting the handlers list.
 */
static pthread_mutex_t atexit_mutex = PTHREAD_MUTEX_INITIALIZER;
/**
 * Stack of cleanup handlers.  These will be invoked in reverse order when 
 */
static struct quick_exit_handler *handlers;

int
at_quick_exit(void (*func)(void))
{
	size_t page_size = getpagesize();
	struct quick_exit_handler *h;

	pthread_mutex_lock(&atexit_mutex);

	if (pool_size < sizeof(*h)) {
		void *ptr = mmap(NULL, page_size, PROT_READ|PROT_WRITE,
		    MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
		if (ptr == MAP_FAILED) {
			pthread_mutex_unlock(&atexit_mutex);
			return (1);
		}
		prctl(PR_SET_VMA, PR_SET_VMA_ANON_NAME, ptr, page_size,
		    "at_quick_exit handlers");
		pool_page = pool = ptr;
		pool_size = page_size;
	} else {
		if (mprotect(pool_page, page_size, PROT_READ|PROT_WRITE)) {
			pthread_mutex_unlock(&atexit_mutex);
			return (1);
		}
	}

	h = pool++;
	pool_size -= sizeof(*h);

	h->cleanup = func;

	h->next = handlers;
	handlers = h;

	mprotect(pool_page, page_size, PROT_READ);

	pthread_mutex_unlock(&atexit_mutex);
	return (0);
}

void
quick_exit(int status)
{
	struct quick_exit_handler *h;

	/*
	 * XXX: The C++ spec requires us to call std::terminate if there is an
	 * exception here.
	 */
	for (h = handlers; NULL != h; h = h->next)
		h->cleanup();
	_Exit(status);
}
