// SPDX-License-Identifier: GPL-2.0-only
/*
 * tlsf wrapper for barebox
 *
 * Copyright (C) 2011 Antony Pavlov <antonynpavlov@gmail.com>
 */

#include <malloc.h>
#include <string.h>

#include <stdio.h>
#include <module.h>
#include <tlsf.h>

extern tlsf_t tlsf_mem_pool;

void *malloc(size_t bytes)
{
	void *mem;
	/*
	 * tlsf_malloc returns NULL for zero bytes, we instead want
	 * to have a valid pointer.
	 */
	if (!bytes)
		bytes = 1;

	mem = tlsf_malloc(tlsf_mem_pool, bytes);
	if (!mem)
		errno = ENOMEM;

	return mem;
}
EXPORT_SYMBOL(malloc);

void free(void *mem)
{
	tlsf_free(tlsf_mem_pool, mem);
}
EXPORT_SYMBOL(free);

void *realloc(void *oldmem, size_t bytes)
{
	void *mem = tlsf_realloc(tlsf_mem_pool, oldmem, bytes);
	if (!mem)
		errno = ENOMEM;

	return mem;
}
EXPORT_SYMBOL(realloc);

void *memalign(size_t alignment, size_t bytes)
{
	void *mem = tlsf_memalign(tlsf_mem_pool, alignment, bytes);
	if (!mem)
		errno = ENOMEM;

	return mem;
}
EXPORT_SYMBOL(memalign);

struct malloc_stats {
	size_t nfree;
	size_t free;
	size_t nused;
	size_t used;
	size_t total;
};

static void malloc_walker(void* ptr, size_t size, int used, void *user)
{
	struct malloc_stats *s = user;

	if (used) {
		s->nused++;
		s->used += size;
	} else {
		s->nfree++;
		s->free += size;
	}
	s->total += size;
}

//extern bool added_ram3;
//extern pool_t tlsf_mem_pool_3;

void malloc_stats(void)
{
	struct malloc_stats s = { 0 };;

	s.used = 0;
	s.free = 0;

	pool_t pool = tlsf_get_pool(tlsf_mem_pool);

	while (pool) {
		tlsf_walk_pool(pool, malloc_walker, &s);

		printf("pool @ %p:\n", pool);
		printf("num used: %10zu | size used: %10zu\n", s.nused, s.used);
		printf("num free: %10zu | size free: %10zu\n", s.nfree, s.free);
		printf("total size: %zu\n", s.total);

		pool = tlsf_get_next_pool(pool);
	}

#if 0
	memset(&s, 0, sizeof(s));
	tlsf_walk_pool(tlsf_get_pool(tlsf_mem_pool), malloc_walker, &s);
	printf("first pool @ %p:\n", tlsf_get_pool(tlsf_mem_pool));
	printf("num used: %10zu | size used: %10zu\n", s.nused, s.used);
	printf("num free: %10zu | size free: %10zu\n", s.nfree, s.free);
	printf("total size: %zu\n", s.total);

	if (added_ram3) {
		printf("walking ram3 pool\n");
		tlsf_walk_pool(tlsf_mem_pool_3, malloc_walker, &s);

		printf("second pool @ %p:\n", tlsf_mem_pool_3);
		printf("num used: %10zu | size used: %10zu\n", s.nused, s.used);
		printf("num free: %10zu | size free: %10zu\n", s.nfree, s.free);
		printf("total size: %zu\n", s.total);
	}
#endif
}
