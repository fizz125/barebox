
#include <common.h>
#include <command.h>

#include <memory.h>

#include <linux/types.h>

#include <tlsf.h>

extern void *ram2;
extern void *ram3;

extern tlsf_t tlsf_mem_pool;

bool added_ram2 = false;
bool added_ram3 = false;

pool_t tlsf_mem_pool_3;
pool_t tlsf_mem_pool_2;

static char *chunks[24];
static size_t next_idx = 0;

static int do_allocate_chunk(int argc, char *argv[])
{
	chunks[next_idx] = malloc(1024 * 1024);
	if (!chunks[next_idx]) {
		printf("Allocate failed!\n");
		return COMMAND_ERROR;
	}

	printf("Chunk allocated @ %p\n", chunks[next_idx]);
	next_idx++;

	return COMMAND_SUCCESS;
}

BAREBOX_CMD_START(alloc_test)
	.cmd	= do_allocate_chunk,
	BAREBOX_CMD_DESC("allocate a (1MiB ?) chunk of memory\n")
	BAREBOX_CMD_GROUP(CMD_GRP_MEM)
BAREBOX_CMD_END

static int do_add_pool(int argc, char *argv[])
{

	if (!added_ram3) {
//		tlsf_mem_pool_3 = tlsf_add_pool(tlsf_mem_pool, ram3, CONFIG_MALLOC_SIZE);
		tlsf_mem_pool_3 = add_tlsf_pool(ram3, CONFIG_MALLOC_SIZE);
		added_ram3 = true;
		printf("added ram3\n");
		return COMMAND_SUCCESS;
	} else
		printf("Cannot add ram3 a second time\n");

	if (!added_ram2) {
//		tlsf_mem_pool_2 = tlsf_add_pool(tlsf_mem_pool, ram2, CONFIG_MALLOC_SIZE);
		tlsf_mem_pool_2 = add_tlsf_pool(ram2, CONFIG_MALLOC_SIZE);
		added_ram2 = true;
		printf("added ram2\n");
		return COMMAND_SUCCESS;
	} else
		printf("Cannot add ram2 a second time\n");

//	tlsf_remove_pool(tlsf_mem_pool, tlsf_mem_pool_3);
	remove_tlsf_pool(tlsf_mem_pool_3);
	printf("ram3 removed\n");

	return COMMAND_SUCCESS;
}

BAREBOX_CMD_START(add_pool)
	.cmd	= do_add_pool,
	BAREBOX_CMD_DESC("add more memory to tlsf pool")
	BAREBOX_CMD_GROUP(CMD_GRP_MEM)
BAREBOX_CMD_END

