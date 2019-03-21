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

#define PMEM_MAX_SIZE (1024 * 1024 * 64)
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

	sprintf(buf, "astats.arenas.%d.pactive", arena);
	memkind_default_mallctl(buf, &size, &len, NULL, 0);
	printf("pages active: %zu\n", size);
	memkind_default_mallctl("stats.active", &size, &len, NULL, 0);
	printf("total number of bytes in active pages allocated by the application: %zu\n", size);
	memkind_default_mallctl("stats.mapped", &size, &len, NULL, 0);
	printf("number of bytes in active extents: %zu\n", size);
	sprintf(buf, "stats.arenas.%d.mapped", arena);
	memkind_default_mallctl(buf, &size, &len, NULL, 0);
	printf("mapped bytes in arena: %zu\n", size);
	sprintf(buf, "stats.arenas.%d.small.allocated", arena);
	memkind_default_mallctl(buf, &size, &len, NULL, 0);
	printf("allocated bytes of small obj: %zu\n", size);
	for (int i = 0; i < 36; i++)
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
		
		//long int bin = (long long)size << 16 / availregs;
		//printf("%d. bin util: %ld\n", i, bin);

		sprintf(buf, "stats.arenas.%d.bins.%d.nslabs", arena, i);
		memkind_default_mallctl(buf, &number, &len, NULL, 0);
		//printf("%d. nslabs: %d\n", i, number);

		sprintf(buf, "stats.arenas.%d.bins.%d.nreslabs", arena, i);
		memkind_default_mallctl(buf, &number, &len, NULL, 0);
		printf("%d. reslabs: %d\n", i, number);
	}
}

TEST_F(MemkindDefaultGetDefragHintTests, test_TC_mctltest)
{
	struct memkind *kind = NULL;
	int err = memkind_create_pmem(PMEM_DIR, PMEM_MAX_SIZE, &kind);
	if (err != 0)
		printf("err = %d\n", err); 
	unsigned int arena;
	kind->ops->get_arena(kind, &arena, 1 * KB);
	
	printf("arena: %d\n", arena);
	//size_t len;
	//size_t size;
	//unsigned int number;
	//char buf[50];

	int bin, run;
	//long long bin_util[2][36];
	//long long run_util[2][36];*/
	int alloc = 50000;
	void* ptr[alloc];
	printf("Init:\n");
	printf("arena_zero: %d\n", kind->arena_zero);
	printStats(arena);
	
	for (int i = 0; i < alloc; i++)
	{
		ptr[i] = memkind_calloc(kind, 1,1 * KB);
		const char str[395] = "qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq";
		**ptr[i] = str;
	}
	printf("\nAfter %d malloc:\n", alloc);
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
	printStats(arena);
	//for (int i = 0; i < 36; i++)
	//{
	//	stats.arenas.<i>.bins.<j>.nslabs // culminative number of slabs created
	//	int availregs = 0;
	//	sprintf(buf, "arenas.bin.%d.nregs", i);
	//	memkind_default_mallctl(buf, &number, &len, NULL, 0);
	//	availregs = number;
	//	sprintf(buf, "stats.arenas.%d.bins.%d.curslabs",arena, i);
	//	memkind_default_mallctl(buf, &number, &len, NULL, 0);
	//	availregs *= number;
	//	printf("%d. availregs = %d\n", i, availregs);
	//	sprintf(buf, "stats.arenas.%d.bins.%d.curregs",arena, i);
	//	memkind_default_mallctl(buf, &size, &len, NULL, 0);
	//	bin_util[0][i] = (long long)size << 16 / availregs;
	//}
	for (int i = 0; i < alloc; i++)
	{
		if (ptr[i] != NULL)
		{
			//int availregs = 0;
			//sprintf(buf, "arenas.bin.0.nregs");
			//memkind_default_mallctl(buf, &number, &len, NULL, 0);
			//printf("nregs: %d\n", number);
			//sprintf(buf, "stats.arenas.255.bins.0.curslabs");
			//memkind_default_mallctl(buf, &number, &len, NULL, 0);
			//printf("curslabs: %d", number);
			//printf("availregs = %d = %d * %d \n vs \n", availregs * number, availregs, number);
			int hint = memkind_default_get_defrag_hint(ptr[i], &bin, &run);
			if (hint)
			{
				// Condition from Redis
				if (run > bin || run == 1 << 16)
				{}
				else
				{
					//printf("i: %d\n", i);
					//printf("1. run: %d\n", run);
					//printf("1. bin: %d\n", bin);
					void* newptr = memkind_calloc(kind,1, 1 * KB);
					memcpy(newptr, ptr[i], 1 * KB);
					memkind_free(kind, ptr[i]);
					ptr[i] = newptr;
					////memkind_free(kind, newptr);
					//hint = memkind_default_get_defrag_hint(ptr[i], &bin, &run);
					//printf("2. run: %d\n", run);
					//printf("2. bin: %d\n", bin);
				}
			}
			//availregs = 0;
			//sprintf(buf, "arenas.bin.0.nregs");
			//memkind_default_mallctl(buf, &number, &len, NULL, 0);
			//printf("nregs: %d\n", number);
			//sprintf(buf, "stats.arenas.255.bins.0.curslabs");
			//memkind_default_mallctl(buf, &number, &len, NULL, 0);
			//printf("curslabs: %d", number);
			//printf("availregs = %d = %d * %d \n\n", availregs * number, availregs, number);
		}
	}
	printf("\nAfter defrag:\n");
	printStats(arena);
	//for (int i = 0; i < 36; i++)
	//{
	//	int availregs = 0;
	//	sprintf(buf, "arenas.bin.%d.nregs", i);
	//	memkind_default_mallctl(buf, &number, &len, NULL, 0);
	//	availregs = number;
	//	sprintf(buf, "stats.arenas.%d.bins.%d.curslabs", arena, i);
	//	memkind_default_mallctl(buf, &number, &len, NULL, 0);
	//	availregs *= number;
	//	printf("%d. availregs = %d\n", i, availregs);
	//	sprintf(buf, "stats.arenas.%d.bins.%d.curregs", arena, i);
	//	memkind_default_mallctl(buf, &size, &len, NULL, 0);
	//	bin_util[1][i] = (long long)size << 16 / availregs;
	//}

	//for (int i = 0; i < 36; i++)
	//{
	//	if (bin_util[0][i] > bin_util[1][i])
	//		printf("%lld > %lld\n", bin_util[0][i], bin_util[1][i]);
	//	else
	//		printf("%lld <= %lld\n", bin_util[0][i], bin_util[1][i]);
	//}
	for (int i = 1; i < alloc; i++)
	{
		memkind_free(kind, ptr[i]);
	}

	memkind_destroy_kind(kind);
}
