// SPDX-License-Identifier: GPL-2.0-only
/*
 * tlsf wrapper for barebox
 *
 * Copyright (C) 2011 Antony Pavlov <antonynpavlov@gmail.com>
 */

#include <malloc.h>
#include <string.h>

#include <stdio.h>
#include <memory.h>
#include <module.h>
#include <tlsf.h>

#include <linux/kasan.h>
#include <linux/list.h>

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

struct pool_entry {
	pool_t pool;
	struct list_head list;
};

static LIST_HEAD(mem_pool_list);

pool_t add_tlsf_pool(void *mem, size_t bytes)
{
	pool_t new_pool;
	struct pool_entry *new_pool_entry;

	struct resource *resptr;
	resptr = request_sdram_region("extended malloc space", (uintptr_t)mem, bytes);
	if (IS_ERR(resptr)) {
		printf("request_sdram_region error\n");
		return 0;
	}

	if (!(new_pool = tlsf_add_pool(tlsf_mem_pool, mem, bytes)))
		return 0;

	kasan_poison_shadow(mem, bytes, KASAN_TAG_INVALID);

	new_pool_entry = xzalloc(sizeof(struct pool_entry));
	new_pool_entry->pool = new_pool;

	list_add(&(new_pool_entry->list), &mem_pool_list);

	return new_pool;
}

#if 0
/* if the pool is not empty, tlsf_remove_pool will fail
 * But if that happens, there's no error returns just a console print by
 * tlsf_assert
 * We can attempt to confirm the pool is empty first by walking it, but if the
 * struct pool_entry was allocated from the pool it'll basically just deadlock
 *
 * Alternatively, we could modify add_tlsf_pool to allocate the tlsf_malloc
 * tracking struct before adding the pool. Which could fail in the narrow case
 * where we're out of memory but trying to add more. Thats mostly just moving
 * the issue from the "remove" step to the "add" step, but it's equally as rare
 */
static void __remove_pool(struct pool_entry *candidate_pool)
{
	struct malloc_stats s;
	pool_t pool = candidate_pool->pool;

	s.used = 0;
	s.free = 0;

	tlsf_walk_pool(pool, malloc_walker, &s);
	if (s.used > 0)
		return;

	list_del(&(candidate_pool->list));
	free(candidate_pool);
	tlsf_remove_pool(tlsf_mem_pool, pool);
}
#endif

void remove_tlsf_pool(pool_t pool)
{
	struct pool_entry *cur_pool;

	list_for_each_entry(cur_pool, &mem_pool_list, list) {
		if (cur_pool->pool == pool) {
			list_del(&(cur_pool->list));
			free(cur_pool);
			tlsf_remove_pool(tlsf_mem_pool, pool);
			return;
		}
	}
}

struct malloc_stats {
	size_t free;
	size_t used;
};

static void malloc_walker(void* ptr, size_t size, int used, void *user)
{
	struct malloc_stats *s = user;

	if (used)
		s->used += size;
	else
		s->free += size;
}

void malloc_stats(void)
{
	struct pool_entry *cur_pool;
	struct malloc_stats s;

	s.used = 0;
	s.free = 0;

	tlsf_walk_pool(tlsf_get_pool(tlsf_mem_pool), malloc_walker, &s);

	list_for_each_entry(cur_pool, &mem_pool_list, list)
		tlsf_walk_pool(cur_pool->pool, malloc_walker, &s);

	printf("used: %zu\nfree: %zu\n", s.used, s.free);
}
