/*
 * Copyright (C) 2019 Intel Corporation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice(s),
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice(s),
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER(S) ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO
 * EVENT SHALL THE COPYRIGHT HOLDER(S) BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include <memkind.h>
#include <memkind/internal/memkind_default.h>
#include <stdio.h>

#include <sys/stat.h>
#include <fcntl.h>

#include "common.h"

#define PMEM_MAX_SIZE (1024 * 1024 * 1024)
extern const char *PMEM_DIR;

class MemkindDefaultGetDefragHintTests : public ::testing::Test
{};

bool StartDefrag(int bin, int run)
{
	if (run > bin || run == 1 << 16)
		return false;
	else
		return true;
}

void *try_defrag(struct memkind *kind, void *ptr, bool tcache)
{
	bool tcache_state = tcache;
	size_t tcache_size = sizeof(bool);
	int bin_util;
	int run_util;
	size_t size;

	if (ptr == NULL)
		return NULL;

	if (!memkind_default_get_defrag_hint(ptr, &bin_util, &run_util))
		return ptr;

	if (!StartDefrag(bin_util,run_util))
		return ptr;

	int err = memkind_default_mallctl("thread.tcache.enabled", NULL, NULL, &tcache_state, tcache_size);
	if (err != 0)
		printf("Can't change tcache, err: %d\n", err);
	
	size = memkind_malloc_usable_size(kind, ptr);
	void *newptr = memkind_malloc(kind, size);
	memcpy(newptr, ptr, size);
	memkind_free(kind, ptr);
	tcache_state = true;
	err = memkind_default_mallctl("thread.tcache.enabled", NULL, NULL, &tcache_state, tcache_size);
	if (err != 0)
		printf("Can't change tcache, err: %d\n", err);
	return newptr;
}

void printStats(unsigned int arena)
{
	size_t len;
	size_t size;
	unsigned int number;
	char buf[50];

	int nbins;
	memkind_default_mallctl("arenas.nbins", &nbins, &len, NULL, 0);
	//printf("nbins: %d\n", nbins);
	sprintf(buf, "stats.arenas.%d.pactive", arena);
	memkind_default_mallctl(buf, &size, &len, NULL, 0);
	printf("pages active: %zu\n", size);
	memkind_default_mallctl("stats.active", &size, &len, NULL, 0);
	//printf("total number of bytes in active pages allocated by the application: %zu\n", size);
	memkind_default_mallctl("stats.mapped", &size, &len, NULL, 0);
	//printf("number of bytes in active extents: %zu\n", size);
	sprintf(buf, "stats.arenas.%d.mapped", arena);
	memkind_default_mallctl(buf, &size, &len, NULL, 0);
	//printf("mapped bytes in arena: %zu\n", size);
	sprintf(buf, "stats.arenas.%d.small.allocated", arena);
	memkind_default_mallctl(buf, &size, &len, NULL, 0);
	printf("allocated bytes of small obj: %zu\n", size);
	for (int i = 0; i < nbins; i++)
	{
		int availregs = 0;
		sprintf(buf, "arenas.bin.%d.nregs", i);
		memkind_default_mallctl(buf, &number, &len, NULL, 0);
		//printf("%d. nregs[bin %d]: %d\n", i,i, number);
		availregs = number;

		sprintf(buf, "stats.arenas.%d.bins.%d.curslabs", arena, i);
		memkind_default_mallctl(buf, &number, &len, NULL, 0);
		//printf("%d. current slabs number (arena %d, bin %d): %d\n", i, arena, i, number);
		availregs *= number;

		//printf("%d. availregs = %d\n",i, availregs);
		sprintf(buf, "stats.arenas.%d.bins.%d.curregs", arena, i);
		memkind_default_mallctl(buf, &size, &len, NULL, 0);
		//printf("%d. curregs [bin %d]:%zu\n", i,i,size << 16);

		if (availregs == 0)
		{
			//printf("%d. bin util: 0\n", i);
		}
		else
		{
			long int bin = (long long)size << 16 / availregs;
			printf("%d. bin util: %ld\n", i, bin);
		}

		sprintf(buf, "stats.arenas.%d.bins.%d.nslabs", arena, i);
		memkind_default_mallctl(buf, &number, &len, NULL, 0);
		//printf("%d. nslabs: %d\n", i, number);

		sprintf(buf, "stats.arenas.%d.bins.%d.nreslabs", arena, i);
		memkind_default_mallctl(buf, &number, &len, NULL, 0);
		//printf("%d. reslabs: %d\n", i, number);
	}
}

void UpdateStats()
{
	// Update the statistics cached by mallctl.
	uint64_t epoch = 1;
	size_t sz = sizeof(epoch);
	memkind_default_mallctl("epoch", &epoch, &sz, &epoch, sz);
}

TEST_F(MemkindDefaultGetDefragHintTests, test_TC_MEMKIND_GetDefragHintReturnFalse)
{
	struct memkind *kind = nullptr;

	int err = memkind_create_pmem(PMEM_DIR, PMEM_MAX_SIZE, &kind);
	ASSERT_EQ(err, 0);

	int bin = -1;
	int run = -1;
	int hint = -1;
	int alloc = 50000;
	void *ptr[alloc];

	for (int i = 0; i < alloc; i++) {
		ptr[i] = memkind_malloc(kind, 128);
		ASSERT_NE(ptr[i], nullptr);
	}

	hint = memkind_default_get_defrag_hint(ptr[alloc-1], &bin, &run);
	ASSERT_EQ(hint, 0);

	for (int i = 0; i < alloc; i++) {
		memkind_free(kind, ptr[i]);
	}

	ptr[0] = memkind_malloc(kind, 50 * KB);
	ASSERT_NE(ptr[0], nullptr);
	hint = memkind_default_get_defrag_hint(ptr[0], &bin, &run);
	ASSERT_EQ(hint, 0);
	memkind_free(kind, ptr[0]);

	memkind_destroy_kind(kind);
	ASSERT_EQ(err, 0);
}

TEST_F(MemkindDefaultGetDefragHintTests, test_TC_MEMKIND_GetDefragHintReturnTrue)
{
	struct memkind *kind = nullptr;

	int err = memkind_create_pmem(PMEM_DIR, PMEM_MAX_SIZE, &kind);
	ASSERT_EQ(err, 0);

	int bin = -1;
	int run = -1;
	int hint = -1;
	int alloc = 50000;
	void *ptr[alloc];

	for (int i = 0; i < alloc; i++) {
		ptr[i] = memkind_malloc(kind, 128);
		ASSERT_NE(ptr[i], nullptr);
	}

	hint = memkind_default_get_defrag_hint(ptr[alloc / 2], &bin, &run);
	ASSERT_EQ(hint, 1);

	for (int i = 0; i < alloc; i++) {
		memkind_free(kind, ptr[i]);
	}

	memkind_destroy_kind(kind);
	ASSERT_EQ(err, 0);
}

TEST_F(MemkindDefaultGetDefragHintTests, test_TC_MEMKIND_GetDefragHintCheckBinUtilizationReturnFalse)
{
	struct memkind *kind = nullptr;

	int err = memkind_create_pmem(PMEM_DIR, PMEM_MAX_SIZE, &kind);
	ASSERT_EQ(err, 0);
	
	int bin = -1;
	int run = -1;
	int hint = -1;
	int alloc = 50000;
	void *ptr[alloc];
	bool startDefrag;

	for (int i = 0; i < alloc; i++) {
		ptr[i] = memkind_malloc(kind, 1 * KB);
		ASSERT_NE(ptr[i], nullptr);
	}

	hint = memkind_default_get_defrag_hint(ptr[alloc / 2], &bin, &run);
	ASSERT_EQ(hint, 1);
	startDefrag = StartDefrag(bin, run);
	ASSERT_EQ(startDefrag, 0);

	for (int i = 0; i < alloc; i++) {
		memkind_free(kind, ptr[i]);
	}

	memkind_destroy_kind(kind);
	ASSERT_EQ(err, 0);
}

TEST_F(MemkindDefaultGetDefragHintTests, test_TC_MEMKIND_GetDefragHintCheckBinUtilizationReturnTrue)
{
	struct memkind *kind = nullptr;

	int err = memkind_create_pmem(PMEM_DIR, PMEM_MAX_SIZE, &kind);
	ASSERT_EQ(err, 0);

	int bin = -1;
	int run = -1;
	int hint = -1;
	int alloc = 50000;
	void *ptr[alloc];
	bool startDefrag;

	for (int i = 0; i < alloc; i++) {
		ptr[i] = memkind_malloc(kind, 1 * KB);
		ASSERT_NE(ptr[i], nullptr);
	}

	for (int i = 0; i < alloc;) {
		memkind_free(kind, ptr[i]);
		ptr[i] = nullptr;
		// Free memory in irregular pattern
		if (i % 2 == 0)
			i += 3;
		else
			i += 5;
	}
	
	hint = memkind_default_get_defrag_hint(ptr[10], &bin, &run);
	ASSERT_EQ(hint, 1);
	startDefrag = StartDefrag(bin, run);
	ASSERT_EQ(startDefrag, 1);

	for (int i = 0; i < alloc; i++) {
		memkind_free(kind, ptr[i]);
	}

	memkind_destroy_kind(kind);
	ASSERT_EQ(err, 0);
}

TEST_F(MemkindDefaultGetDefragHintTests, test_TC_mctltest_standard)
{
	struct memkind *kind = NULL;
	int err = memkind_create_pmem(PMEM_DIR, PMEM_MAX_SIZE, &kind);
	ASSERT_EQ(err, 0);

	unsigned int arena;
	int freeCalled = 0;
	int alloc = 100000;
	void* ptr[alloc];
	printf("Init:\n");
	kind->ops->get_arena(kind, &arena, 2 * KB);
	printf("arena: %d\n", arena);
	UpdateStats();
	printStats(arena);

	for (int i = 0; i < alloc; i++)
	{
		ptr[i] = memkind_malloc(kind,2 * KB);
	}
	printf("\nAfter %d malloc:\n", alloc);
	UpdateStats();
	printStats(arena);
	for (int i = 0; i < alloc;)
	{
		memkind_free(kind, ptr[i]);
		ptr[i] = NULL;
		// Free memory with irregular pattern
		if (i % 2 == 0)
			i += 3;
		else
			i += 5;
		freeCalled++;
	}
	printf("\nAfter free:\n");
	printf("Free called: %d\n", freeCalled);
	UpdateStats();
	printStats(arena);
	
	for (int i = 0; i < alloc; i++)
	{
		ptr[i] = try_defrag(kind, ptr[i], false);
	}
	printf("\nAfter defrag:\n");
	UpdateStats();
	printStats(arena);

	for (int i = 0; i < alloc; i++)
	{
		memkind_free(kind, ptr[i]);
	}

	memkind_destroy_kind(kind);
}

TEST_F(MemkindDefaultGetDefragHintTests, test_TC_mctltest_multiple_sizes_standard)
{
	struct memkind *kind = NULL;
	int err = memkind_create_pmem(PMEM_DIR, PMEM_MAX_SIZE, &kind);
	ASSERT_EQ(err, 0);

	int freeCalled = 0;
	unsigned int arena;
	unsigned int arenaLookup;
	int alloc = 100000;
	void* ptr[alloc];
	printf("Init:\n");
	kind->ops->get_arena(kind, &arena, 1 * KB);
	printf("arena: %d\n", arena);
	UpdateStats();
	printStats(arena);

	for (int i = 0; i < alloc; i++)
	{
		if (i < alloc / 5)
		{
			ptr[i] = memkind_malloc(kind, 1 * KB);

			kind->ops->get_arena(kind, &arenaLookup, 1 * KB);
			ASSERT_EQ((int)arena, (int)arenaLookup);
		}
		else if(i < (alloc/5)*2)
		{
			ptr[i] = memkind_malloc(kind, 3 * KB);

			kind->ops->get_arena(kind, &arenaLookup, 3 * KB);
			ASSERT_EQ((int)arena, (int)arenaLookup);
		}
		else if(i < (alloc/5)*3)
		{
			ptr[i] = memkind_malloc(kind, 4 * KB);

			kind->ops->get_arena(kind, &arenaLookup, 4 * KB);
			ASSERT_EQ((int)arena, (int)arenaLookup);
		}
		else if(i < (alloc/5)*4)
		{
			ptr[i] = memkind_malloc(kind, 256);

			kind->ops->get_arena(kind, &arenaLookup, 256);
			ASSERT_EQ((int)arena, (int)arenaLookup);
		}
		else if( i < alloc)
		{
			ptr[i] = memkind_malloc(kind, 512);

			kind->ops->get_arena(kind, &arenaLookup, 512);
			ASSERT_EQ((int)arena, (int)arenaLookup);
		}
	}
	printf("\nAfter %d malloc:\n", alloc);
	UpdateStats();
	printStats(arena);
	for (int i = 0; i < alloc;)
	{
		memkind_free(kind, ptr[i]);
		ptr[i] = NULL;
		// Free memory with irregular pattern
		if (i % 2 == 0)
			i += 3;
		else
			i += 5;
		freeCalled++;
	}
	printf("\nAfter free:\n");
	printf("Free called: %d\n", freeCalled);
	UpdateStats();
	printStats(arena);

	for (int i = 0; i < alloc; i++)
	{
		ptr[i] = try_defrag(kind, ptr[i], false);
	}
	printf("\nAfter defrag:\n");
	UpdateStats();
	printStats(arena);

	for (int i = 0; i < alloc; i++)
	{
		memkind_free(kind, ptr[i]);
	}

	memkind_destroy_kind(kind);
}

TEST_F(MemkindDefaultGetDefragHintTests, test_TC_mctltest_default)
{
	unsigned int arena = 0;
	int freeCalled = 0;
	size_t sarena = sizeof(arena);
	int alloc = 100000;
	void* ptr[alloc];
	printf("Init:\n");
	printf("arena: %d\n", arena);
	UpdateStats();
	printStats(arena);
	
	for (int i = 0; i < alloc; i++)
	{
		ptr[i] = memkind_malloc(MEMKIND_DEFAULT, 1 * KB);
		
		size_t ptrsize = memkind_malloc_usable_size(MEMKIND_DEFAULT,ptr[i]);
		if (memkind_default_mallctl("arenas.lookup", &arena, &sarena, &ptr[i], ptrsize) != 0)
			ASSERT_EQ((int)arena, 0);
		else
			printf("arena lookup err\n");
	}
	printf("\nAfter %d malloc:\n", alloc);
	UpdateStats();
	printStats(arena);
	for (int i = 0; i < alloc;)
	{
		memkind_free(MEMKIND_DEFAULT, ptr[i]);
		ptr[i] = NULL;
		// Free memory with irregular pattern
		if (i % 2 == 0)
			i += 3;
		else
			i += 5;
		freeCalled++;
	}
	printf("\nAfter free:\n");
	printf("Free called: %d\n", freeCalled);
	UpdateStats();
	printStats(arena);
	
	for (int i = 0; i < alloc; i++)
	{
		ptr[i] = try_defrag(MEMKIND_DEFAULT, ptr[i], false);
	}
	printf("\nAfter defrag:\n");
	UpdateStats();
	printStats(arena);
	
	for (int i = 0; i < alloc; i++)
	{
		memkind_free(MEMKIND_DEFAULT, ptr[i]);
	}
}

TEST_F(MemkindDefaultGetDefragHintTests, test_TC_mctltest_config)
{
	struct memkind *kind = NULL;
	struct memkind_config *test_cfg = memkind_config_new();
	memkind_config_set_path(test_cfg, PMEM_DIR);
	memkind_config_set_size(test_cfg, PMEM_MAX_SIZE);
	memkind_config_set_memory_usage_policy(test_cfg,
		MEMKIND_MEM_USAGE_POLICY_CONSERVATIVE);
	int err = memkind_create_pmem_with_config(test_cfg, &kind);
	ASSERT_EQ(err, 0);

	unsigned int arena;
	int alloc = 100000;
	void* ptr[alloc];
	printf("Init:\n");
	kind->ops->get_arena(kind, &arena, 1 * KB);
	printf("arena: %d\n", arena);
	UpdateStats();
	printStats(arena);

	for (int i = 0; i < alloc; i++)
	{
		ptr[i] = memkind_malloc(kind, 1 * KB);
	}
	printf("\nAfter %d malloc:\n", alloc);
	UpdateStats();
	printStats(arena);
	for (int i = 0; i < alloc;)
	{
		memkind_free(kind, ptr[i]);
		ptr[i] = NULL;
		// Free memory with irregular pattern
		if (i % 2 == 0)
			i += 3;
		else
			i += 5;
	}
	printf("\nAfter free:\n");
	UpdateStats();
	printStats(arena);

	for (int i = 0; i < alloc; i++)
	{
		ptr[i] = try_defrag(kind, ptr[i], false);
	}
	printf("\nAfter defrag:\n");
	UpdateStats();
	printStats(arena);

	for (int i = 0; i < alloc; i++)
	{
		memkind_free(kind, ptr[i]);
	}

	memkind_destroy_kind(kind);
}

TEST_F(MemkindDefaultGetDefragHintTests, test_TC_mctltest_multiple_sizes_config)
{
	struct memkind *kind = NULL;
	struct memkind_config *test_cfg = memkind_config_new();
	memkind_config_set_path(test_cfg, PMEM_DIR);
	memkind_config_set_size(test_cfg, PMEM_MAX_SIZE);
	memkind_config_set_memory_usage_policy(test_cfg,
		MEMKIND_MEM_USAGE_POLICY_CONSERVATIVE);
	int err = memkind_create_pmem_with_config(test_cfg, &kind);
	ASSERT_EQ(err, 0);

	int freeCalled = 0;
	unsigned int arena;
	int alloc = 100000;
	void* ptr[alloc];
	printf("Init:\n");
	kind->ops->get_arena(kind, &arena, 1 * KB);
	printf("arena: %d\n", arena);
	UpdateStats();
	printStats(arena);

	for (int i = 0; i < alloc; i++)
	{
		if (i < alloc / 5)
		{
			ptr[i] = memkind_malloc(kind, 1 * KB);
		}
		else if (i < (alloc / 5) * 2)
		{
			ptr[i] = memkind_malloc(kind, 3 * KB);
		}
		else if (i < (alloc / 5) * 3)
		{
			ptr[i] = memkind_malloc(kind, 4 * KB);
		}
		else if (i < (alloc / 5) * 4)
		{
			ptr[i] = memkind_malloc(kind, 256);
		}
		else if (i < alloc)
		{
			ptr[i] = memkind_malloc(kind, 512);
		}
	}
	printf("\nAfter %d malloc:\n", alloc);
	UpdateStats();
	printStats(arena);
	for (int i = 0; i < alloc;)
	{
		memkind_free(kind, ptr[i]);
		ptr[i] = NULL;
		// Free memory with irregular pattern
		if (i % 2 == 0)
			i += 3;
		else
			i += 5;
		freeCalled++;
	}
	printf("\nAfter free:\n");
	printf("Free called: %d\n", freeCalled);
	UpdateStats();
	printStats(arena);

	for (int i = 0; i < alloc; i++)
	{
		ptr[i] = try_defrag(kind, ptr[i], false);
	}
	printf("\nAfter defrag:\n");
	UpdateStats();
	printStats(arena);

	for (int i = 0; i < alloc; i++)
	{
		memkind_free(kind, ptr[i]);
	}

	memkind_destroy_kind(kind);
}

TEST_F(MemkindDefaultGetDefragHintTests, test_TC_mctltest_onepage_sizes_config_conservative)
{
	struct memkind *kind = NULL;
	struct memkind_config *test_cfg = memkind_config_new();
	memkind_config_set_path(test_cfg, PMEM_DIR);
	memkind_config_set_size(test_cfg, PMEM_MAX_SIZE);
	memkind_config_set_memory_usage_policy(test_cfg,
		MEMKIND_MEM_USAGE_POLICY_CONSERVATIVE);
	int err = memkind_create_pmem_with_config(test_cfg, &kind);
	ASSERT_EQ(err, 0);

	int freeCalled = 0;
	unsigned int arena;
	int alloc = 100000;
	void* ptr[alloc];
	printf("Init:\n");
	kind->ops->get_arena(kind, &arena, 2 * KB);
	printf("arena: %d\n", arena);
	UpdateStats();
	printStats(arena);

	for (int i = 0; i < alloc; i++)
	{
		ptr[i] = memkind_malloc(kind, 2 * KB);
	}
	printf("\nAfter %d malloc:\n", alloc);
	UpdateStats();
	printStats(arena);
	int pageNum = 1;
	for (int i = 0; i < alloc; i++)
	{
		memkind_free(kind, ptr[i]);
		ptr[i] = NULL;
		if (pageNum == 3)
		{
			pageNum = 1;
			if (i % 2 == 0)
				i += 3;
			else
				i += 5;
		}
		freeCalled++;
		pageNum++;
	}
	printf("\nAfter free:\n");
	printf("Free called: %d\n", freeCalled);
	UpdateStats();
	printStats(arena);

	for (int i = 0; i < alloc; i++)
	{
		ptr[i] = try_defrag(kind, ptr[i], false);
	}
	printf("\nAfter defrag:\n");
	UpdateStats();
	printStats(arena);

	for (int i = 0; i < alloc; i++)
	{
		memkind_free(kind, ptr[i]);
	}

	memkind_destroy_kind(kind);
}

TEST_F(MemkindDefaultGetDefragHintTests, test_TC_mctltest_onepage_sizes_configDefault)
{
	struct memkind *kind = NULL;
	struct memkind_config *test_cfg = memkind_config_new();
	memkind_config_set_path(test_cfg, PMEM_DIR);
	memkind_config_set_size(test_cfg, PMEM_MAX_SIZE);
	memkind_config_set_memory_usage_policy(test_cfg,
		MEMKIND_MEM_USAGE_POLICY_DEFAULT);
	int err = memkind_create_pmem_with_config(test_cfg, &kind);
	ASSERT_EQ(err, 0);

	int freeCalled = 0;
	unsigned int arena;
	int alloc = 100000;
	void* ptr[alloc];
	printf("Init:\n");
	kind->ops->get_arena(kind, &arena, 2 * KB);
	printf("arena: %d\n", arena);
	UpdateStats();
	printStats(arena);

	for (int i = 0; i < alloc; i++)
	{
		ptr[i] = memkind_malloc(kind, 2 * KB);
	}
	printf("\nAfter %d malloc:\n", alloc);
	UpdateStats();
	printStats(arena);
	int pageNum = 1;
	for (int i = 0; i < alloc; i++)
	{
		memkind_free(kind, ptr[i]);
		ptr[i] = NULL;
		if (pageNum == 3)
		{
			pageNum = 1;
			if (i % 2 == 0)
				i += 3;
			else
				i += 5;
		}
		freeCalled++;
		pageNum++;
	}
	printf("\nAfter free:\n");
	printf("Free called: %d\n", freeCalled);
	UpdateStats();
	printStats(arena);

	for (int i = 0; i < alloc; i++)
	{
		ptr[i] = try_defrag(kind, ptr[i], false);
	}
	printf("\nAfter defrag:\n");
	UpdateStats();
	printStats(arena);

	for (int i = 0; i < alloc; i++)
	{
		memkind_free(kind, ptr[i]);
	}

	memkind_destroy_kind(kind);
}

TEST_F(MemkindDefaultGetDefragHintTests, test_TC_mctltest_config_compare)
{
	struct memkind *kind = NULL;
	struct memkind_config *test_cfg = memkind_config_new();
	memkind_config_set_path(test_cfg, PMEM_DIR);
	memkind_config_set_size(test_cfg, PMEM_MAX_SIZE);
	memkind_config_set_memory_usage_policy(test_cfg,
		MEMKIND_MEM_USAGE_POLICY_CONSERVATIVE);
	int err = memkind_create_pmem_with_config(test_cfg, &kind);
	ASSERT_EQ(err, 0);

	unsigned int arena;
	int freeCalled = 0;
	kind->ops->get_arena(kind, &arena, 1 * KB);
	printf("arena: %d\n", arena);

	int alloc = 100000;
	void* ptr[alloc];

	size_t len = sizeof(size_t);
	size_t initConf, mallocConf, freeConf, defragConf;
	size_t initStand, mallocStand, freeStand, defragStand;

	char buf[50];
	sprintf(buf, "stats.arenas.%d.pactive", arena);
	UpdateStats();
	memkind_default_mallctl(buf, &initConf, &len, NULL, 0);
	printf("pages active: %zu\n", initConf);

	for (int i = 0; i < alloc; i++)
	{
		ptr[i] = memkind_malloc(kind, 1 * KB);
	}

	UpdateStats();
	memkind_default_mallctl(buf, &mallocConf, &len, NULL, 0);
	printf("pages active: %zu\n", mallocConf);

	int pageCount = 1;
	for (int i = 0; i < alloc; i++)
	{
		memkind_free(kind, ptr[i]);
		ptr[i] = NULL;
		if (pageCount == 8)
		{
			pageCount = 1;

			if (i % 2 == 0)
				i += 3;
			else
				i += 5;
		}
		pageCount++;
		freeCalled++;
	}

	printf("Free called: %d\n", freeCalled);
	UpdateStats();
	memkind_default_mallctl(buf, &freeConf, &len, NULL, 0);
	printf("pages active: %zu\n", freeConf);

	for (int i = 0; i < alloc; i++)
	{
		ptr[i] = try_defrag(kind, ptr[i], false);
	}

	UpdateStats();
	memkind_default_mallctl(buf, &defragConf, &len, NULL, 0);
	printf("pages active: %zu\n", defragConf);

	for (int i = 0; i < alloc; i++)
	{
		memkind_free(kind, ptr[i]);
	}

	memkind_destroy_kind(kind);
	
	// Max memory kind

	struct memkind *defaultKind = NULL;
	struct memkind_config *defaultKind_cfg = memkind_config_new();
	memkind_config_set_path(defaultKind_cfg, PMEM_DIR);
	memkind_config_set_size(defaultKind_cfg, PMEM_MAX_SIZE);
	memkind_config_set_memory_usage_policy(defaultKind_cfg,
		MEMKIND_MEM_USAGE_POLICY_DEFAULT);
	err = 0;
	err = memkind_create_pmem_with_config(defaultKind_cfg, &defaultKind);
	ASSERT_EQ(err, 0);

	defaultKind->ops->get_arena(defaultKind, &arena, 1 * KB);
	printf("arena: %d\n", arena);

	UpdateStats();
	memkind_default_mallctl(buf, &initStand, &len, NULL, 0);
	printf("pages active: %zu\n", initStand);

	for (int i = 0; i < alloc; i++)
	{
		ptr[i] = memkind_malloc(defaultKind, 1 * KB);
	}

	UpdateStats();
	memkind_default_mallctl(buf, &mallocStand, &len, NULL, 0);
	printf("pages active: %zu\n", mallocStand);

	freeCalled = 0;
	pageCount = 1;
	for (int i = 0; i < alloc; i++)
	{
		if (pageCount == 8)
		{
			pageCount = 1;

			if (i % 2 == 0)
				i += 3;
			else
				i += 5;
		}
		memkind_free(defaultKind, ptr[i]);
		ptr[i] = NULL;
		pageCount++;
		freeCalled++;
	}
	
	printf("Free called: %d\n", freeCalled);
	UpdateStats();
	memkind_default_mallctl(buf, &freeStand, &len, NULL, 0);
	printf("pages active: %zu\n", freeStand);

	for (int i = 0; i < alloc; i++)
	{
		ptr[i] = try_defrag(defaultKind, ptr[i], false);
	}

	UpdateStats();
	memkind_default_mallctl(buf, &defragStand, &len, NULL, 0);
	printf("pages active: %zu\n", defragStand);

	for (int i = 0; i < alloc; i++)
	{
		memkind_free(defaultKind, ptr[i]);
	}

	memkind_destroy_kind(defaultKind);
}