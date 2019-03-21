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
#include "common.h"

#define PMEM_MAX_SIZE (1024 * 1024 * 1024)
extern const char *PMEM_DIR;

class MemkindDefaultGetDefragHintTests : public ::testing::Test
{};

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

bool StartDefrag(int bin, int run)
{
	// condition from Redis
	if (run > bin || run == 1 << 16)
		return false;
	else
		return true;
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

	for (int i = 1; i < alloc;) {
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
	//printf("allocated bytes of small obj: %zu\n", size);
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
		sprintf(buf, "stats.arenas.%d.bins.%d.curregs", arena,i);
		memkind_default_mallctl(buf, &size, &len, NULL, 0);
		//printf("%d. curregs [bin %d]:%zu\n", i,i,size << 16);
		
		if (availregs == 0)
		{
			printf("%d. bin util: 0 division!\n", i);
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

TEST_F(MemkindDefaultGetDefragHintTests, test_TC_mctltest)
{
	struct memkind *kind = NULL;
	int err = memkind_create_pmem(PMEM_DIR, PMEM_MAX_SIZE, &kind);
	if (err != 0)
		printf("err = %d\n", err); 
	unsigned int arena;
	kind->ops->get_arena(kind, &arena, 2 * KB);
	
	printf("arena: %d\n", arena);
	size_t len;
	//size_t size;
	//unsigned int number;
	unsigned int nbins;
	//char buf[50];
	memkind_default_mallctl("arenas.nbins", &nbins, &len, NULL, 0);
	printf("nbins: %d\n", nbins);
	int bin, run;
	//long long bin_util[2][36];
	//long long run_util[2][36];*/
	int alloc = 100000;
	void* ptr[alloc];
	printf("Init:\n");
	printf("arena_zero: %d\n", kind->arena_zero);
	UpdateStats();
	printStats(arena);

	for (int i = 0; i < alloc; i++)
	{
		ptr[i] = memkind_calloc(kind, 1,2 * KB);
		*(long double*)ptr[i] = 123456789012345.678;
	}
	printf("\nAfter %d malloc:\n", alloc);
	UpdateStats();
	printStats(arena);
	for (int i = 1; i < alloc;)
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
		if (ptr[i] != NULL)
		{
			int hint = memkind_default_get_defrag_hint(ptr[i], &bin, &run);
			if (hint)
			{
				// Condition from Redis
				if (run > bin || run == 1 << 16)
				{}
				else
				{
					void* newptr = memkind_malloc(kind, 2 * KB);
					memcpy(newptr, ptr[i], 2 * KB);
					memkind_free(kind, ptr[i]);
					ptr[i] = newptr;
				}
			}
		}
	}
	printf("\nAfter defrag:\n");
	UpdateStats();
	printStats(arena);

	for (int i = 1; i < alloc; i++)
	{
		memkind_free(kind, ptr[i]);
	}

	memkind_destroy_kind(kind);
}
